#include <iostream>

#include "cli/argument.hh"
#include "cli/parser.hh"

struct Args {
  cli::Description description{
      "Shows env-var fallback with validation: options can be set via CLI "
      "flags or environment variables, and the same validation applies to "
      "both."};
  cli::Help<> help;

  cli::Arg<"threads", 't',
           cli::conversion::integer<int> | cli::validation::range<1, 32> |
               cli::pack::set_once>
      threads{{.help = "Worker threads 1-32 (env: NUM_THREADS)",
               .presence = cli::required,
               .env = "NUM_THREADS"}};

  cli::Arg<"config", 'c',
           cli::conversion::string | cli::validation::matches<".*\\.toml"> |
               cli::pack::set_once>
      config{{.help = "Config file *.toml (env: CONFIG_FILE)",
              .presence = cli::optional,
              .env = "CONFIG_FILE"}};

  cli::Flag<"verbose", 'v'> verbose{
      {.help = "Verbose output (env: VERBOSE)",
       .presence = cli::optional,
       .env = "VERBOSE"}};

  cli::Positional<std::string> job{
      {.help = "Job name to run", .presence = cli::required}};
};

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parse_or_exit<Args>(argc, argv);

  if (args.verbose.value()) {
    std::cout << "verbose: true\n";
  }
  std::cout << "threads: " << *args.threads.value() << '\n';
  if (const auto& cfg = args.config.value()) {
    std::cout << "config: " << *cfg << '\n';
  }
  std::cout << "job: " << *args.job.value() << '\n';
  return 0;
}
