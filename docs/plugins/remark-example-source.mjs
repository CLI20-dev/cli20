import {readFileSync} from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');

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
 * Remark plugin that handles <ExampleSource> at MDX compile time.
 * Reads the C++ source file from disk and injects it as the sourceCode prop.
 */
export default function remarkExampleDocs() {
  return (tree) => {
    visit(tree, 'mdxJsxFlowElement', (node) => {
      if (node.name === 'ExampleSource') {
        handleExampleSource(node);
      }
    });
  };
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
