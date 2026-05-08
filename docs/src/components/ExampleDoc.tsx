import React from 'react';
import CodeBlock from '@theme/CodeBlock';

export type ExampleSourceData = {
  sourcePath: string;
  sourceCode?: string;  // injected by remark plugin at build time
  sourceHref?: string;
};

export type ExampleCommandData = {
  label?: string;
  command: string;
  output?: string;
};

export function ExampleSource({example}: {example: ExampleSourceData}) {
  const sourceHref =
    example.sourceHref ??
    `https://github.com/CLI20-dev/cli20/blob/main/${example.sourcePath}`;
  return (
    <>
      <p>
        Source:{' '}
        <a href={sourceHref}>
          <code>{example.sourcePath}</code>
        </a>
      </p>
      <CodeBlock language="cpp" title={example.sourcePath}>
        {example.sourceCode}
      </CodeBlock>
    </>
  );
}

export function ExampleCommands({
  title = 'Commands',
  items,
  command,
  label,
}: {
  title?: string;
  items?: ExampleCommandData[];
  command?: string;
  label?: string;
}) {
  const normalizedItems =
    items ?? (command ? [{label, command}] satisfies ExampleCommandData[] : []);

  return (
    <>
      <h2>{title}</h2>
      {normalizedItems.map((item, index) => {
        const output = item.output;
        return (
          <div key={`${item.label ?? 'command'}-${index}`} style={{marginBottom: '1.5rem'}}>
            {item.label ? (
              <p>
                <strong>{item.label}</strong>
              </p>
            ) : null}
            <CodeBlock language="bash">{item.command}</CodeBlock>
            {output ? (
              <CodeBlock language="text" title="Observed output">
                {output}
              </CodeBlock>
            ) : null}
          </div>
        );
      })}
    </>
  );
}
