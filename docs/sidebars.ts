import type {SidebarsConfig} from '@docusaurus/plugin-content-docs';

const sidebars: SidebarsConfig = {
  tutorialSidebar: [
    'intro',
    'tutorial',
    {
      type: 'category',
      label: 'Examples',
      link: {
        type: 'doc',
        id: 'examples/index',
      },
      items: [
        {
          type: 'doc',
          id: 'examples/simple-api',
          label: '1. Simple API',
        },
        {
          type: 'doc',
          id: 'examples/subcommand',
          label: '2. Subcommands',
        },
        {
          type: 'doc',
          id: 'examples/validation',
          label: '3. Validation',
        },
        {
          type: 'doc',
          id: 'examples/bind',
          label: '4. Bind',
        },
        {
          type: 'doc',
          id: 'examples/callback-entry-point',
          label: '5. Callback Entry Point',
        },
        {
          type: 'doc',
          id: 'examples/advanced',
          label: '6. Advanced',
        },
        {
          type: 'doc',
          id: 'examples/compiler',
          label: '7. Compiler Wrapper',
        },
        {
          type: 'doc',
          id: 'examples/distribution-and-platform',
          label: '8. Distribution and Platform',
        },
      ],
    },
  ],
};

export default sidebars;
