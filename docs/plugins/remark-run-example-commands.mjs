import {readFileSync} from 'node:fs';
import {spawnSync} from 'node:child_process';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');

function runCommand(command) {
  const result = spawnSync(command, {shell: true, cwd: repoRoot, encoding: 'utf8'});
  return `${result.stdout ?? ''}${result.stderr ?? ''}`.trimEnd();
}

// Minimal AST visitor — avoids adding unist-util-visit as an explicit dependency.
function visit(node, type, fn) {
  if (node?.type === type) fn(node);
  for (const value of Object.values(node)) {
    if (Array.isArray(value)) {
      for (const child of value) {
        if (child && typeof child === 'object' && 'type' in child) {
          visit(child, type, fn);
        }
      }
    } else if (value && typeof value === 'object' && 'type' in value) {
      visit(value, type, fn);
    }
  }
}

// Returns the string value of an estree Literal node, or null.
function literalString(node) {
  return node?.type === 'Literal' && typeof node.value === 'string' ? node.value : null;
}

// Builds an estree Property:  name: "value"
function stringProperty(name, value) {
  return {
    type: 'Property',
    method: false,
    shorthand: false,
    computed: false,
    key: {type: 'Identifier', name},
    value: {type: 'Literal', value, raw: JSON.stringify(value)},
    kind: 'init',
  };
}

function hasProperty(objectExpr, name) {
  return objectExpr.properties.some(p => (p.key?.name ?? p.key?.value) === name);
}

/**
 * Remark plugin that handles <ExampleCommands> and <ExampleSource> at MDX
 * compile time, so no pre-generated files or runtime lookups are needed.
 *
 * <ExampleCommands> — executes each command and injects its output as a prop.
 * <ExampleSource>   — reads the C++ source file and injects it as sourceCode.
 */
export default function remarkExampleDocs() {
  return (tree) => {
    visit(tree, 'mdxJsxFlowElement', (node) => {
      if (node.name === 'ExampleCommands') {
        handleExampleCommands(node);
      } else if (node.name === 'ExampleSource') {
        handleExampleSource(node);
      }
    });
  };
}

function handleExampleCommands(node) {
  for (const attr of node.attributes ?? []) {
    if (attr.type !== 'mdxJsxAttribute') continue;

    // command="./build/examples/foo" — simple string attribute
    if (attr.name === 'command' && typeof attr.value === 'string') {
      node.attributes.push({
        type: 'mdxJsxAttribute',
        name: 'output',
        value: runCommand(attr.value),
      });
      return;
    }

    // items={[{command: '...', label: '...'}]}
    if (attr.name === 'items') {
      const arrayExpr = attr.value?.data?.estree?.body?.[0]?.expression;
      if (arrayExpr?.type !== 'ArrayExpression') continue;

      for (const element of arrayExpr.elements ?? []) {
        if (element?.type !== 'ObjectExpression') continue;

        const commandProp = element.properties.find(
          (p) => (p.key?.name ?? p.key?.value) === 'command',
        );
        const command = literalString(commandProp?.value);
        if (!command || hasProperty(element, 'output')) continue;

        element.properties.push(stringProperty('output', runCommand(command)));
      }
      return;
    }
  }
}

function handleExampleSource(node) {
  const exampleAttr = (node.attributes ?? []).find(
    (a) => a.type === 'mdxJsxAttribute' && a.name === 'example',
  );
  if (!exampleAttr) return;

  const objectExpr = exampleAttr.value?.data?.estree?.body?.[0]?.expression;
  if (objectExpr?.type !== 'ObjectExpression') return;

  if (hasProperty(objectExpr, 'sourceCode')) return;

  const sourcePathProp = objectExpr.properties.find(
    (p) => (p.key?.name ?? p.key?.value) === 'sourcePath',
  );
  const sourcePath = literalString(sourcePathProp?.value);
  if (!sourcePath) return;

  const sourceCode = readFileSync(path.join(repoRoot, sourcePath), 'utf8');
  objectExpr.properties.push(stringProperty('sourceCode', sourceCode));
}
