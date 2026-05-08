import type {ReactNode} from 'react';
import Link from '@docusaurus/Link';
import useBaseUrl from '@docusaurus/useBaseUrl';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';
import CodeBlock from '@theme/CodeBlock';

import styles from './index.module.css';

const HERO_CODE = `\
#include "cli/argument.hh"
#include "cli/parser.hh"

struct Args {
  cli::Flag<"verbose", 'v'>          verbose;
  cli::StringOption<"output", 'o'>   output;
  cli::Positional<std::string,
    cli::nargs::one_or_more>         inputs;
};

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parse_or_exit<Args>(argc, argv);
}`;

const BEFORE_CODE = `\
CLI::App app{"my app"};
bool verbose = false;
std::string output;

app.add_flag("-v,--verbose", verbose);
app.add_option("-o,--output", output);
CLI11_PARSE(app, argc, argv);`;

const AFTER_CODE = `\
struct Args {
  cli::Flag<"verbose", 'v'>        verbose;
  cli::StringOption<"output", 'o'> output;
};

const auto args =
  cli::parse_or_exit<Args>(argc, argv);`;

const FEATURES = [
  {
    icon: '📦',
    title: 'Header-only',
    description:
      'Drop include/ into your project. No linking, no runtime overhead.',
  },
  {
    icon: '⚡',
    title: 'C++20 native',
    description:
      'Compile-time option names, concepts, constexpr. Built for modern C++.',
  },
  {
    icon: '🔒',
    title: 'Type-safe schema',
    description:
      'Your CLI is a struct. No string lookups at runtime.',
  },
  {
    icon: '✨',
    title: 'Zero macros',
    description:
      'Clean modern C++. No registration macros anywhere.',
  },
  {
    icon: '🌿',
    title: 'Subcommands',
    description:
      'Nested structs compose naturally. Recursive by design.',
  },
  {
    icon: '📖',
    title: 'Auto help',
    description:
      'Add cli::Help<> help; and get full formatted help output.',
  },
];

export default function Home(): ReactNode {
  const logoUrl = useBaseUrl('/img/logo.png');
  return (
    <Layout
      title="CLI20 — Define your CLI as a type, not as a builder."
      description="A header-only C++20 typed-schema command-line parser. Define your CLI as a type, not as a builder.">
      <main>
        {/* Hero */}
        <div className={styles.heroBanner}>
          <img
            src={logoUrl}
            alt="CLI20"
            className={styles.heroLogo}
          />
          <p>Define your CLI as a type, not as a builder.</p>
          <div className={styles.heroCode}>
            <CodeBlock language="cpp">{HERO_CODE}</CodeBlock>
          </div>
          <div className={styles.buttons}>
            <Link
              className="button button--primary button--lg"
              to="/docs/intro">
              Get Started
            </Link>
            <Link
              className="button button--secondary button--lg"
              href="https://github.com/CLI20-dev/cli20">
              GitHub
            </Link>
          </div>
        </div>

        {/* Features */}
        <section className={styles.features}>
          <div className={styles.featuresGrid}>
            {FEATURES.map(({icon, title, description}) => (
              <div key={title} className={styles.featureCard}>
                <span className={styles.featureIcon}>{icon}</span>
                <h3>{title}</h3>
                <p>{description}</p>
              </div>
            ))}
          </div>
        </section>

        {/* Comparison */}
        <section className={styles.comparison}>
          <Heading as="h2">CLI11 → CLI20</Heading>
          <div className={styles.comparisonGrid}>
            <div>
              <div className={`${styles.comparisonLabel} ${styles.before}`}>
                Before (CLI11)
              </div>
              <CodeBlock language="cpp">{BEFORE_CODE}</CodeBlock>
            </div>
            <div>
              <div className={`${styles.comparisonLabel} ${styles.after}`}>
                After (CLI20)
              </div>
              <CodeBlock language="cpp">{AFTER_CODE}</CodeBlock>
            </div>
          </div>
        </section>
      </main>
    </Layout>
  );
}
