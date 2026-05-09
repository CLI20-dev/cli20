import React from 'react';
import CodeBlock from '@theme/CodeBlock';

export type ExampleSourceData = {
  sourcePath: string;
  sourceCode?: string;  // injected by remark plugin at build time
  sourceHref?: string;
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
