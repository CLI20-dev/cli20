#include <iostream>

#include "cli/argument.hh"
#include "cli/parser.hh"

namespace fs = std::filesystem;
using namespace cli;

struct Args {
  /* -- Describe the CLI interface using the cli20 API. */
  Description description{
      "Demonstrates the current cli20 parser API with options, positionals, and "
      "a subcommand."};

  /* -- Define the arguments. */
  Help<> help;
  Flag<"quiet", 'q'> verbose;
  IntOption<"count", 'n'> count;

  /* -- Define a positional argument that can be repeated. */
  Positional<std::string, nargs::one_or_more> input_files{};

  /* -- Define a subcommand with its own arguments. */
  struct BuildArgs {
    StringOption<"config", 'c'> config;
    Flag<"release", 'r'> release;
  };
  Command<"build", BuildArgs> build{{.help = "Run the build subcommand"}};
};

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parse_or_exit<Args>(argc, argv);
  std::cout << "verbose: " << std::boolalpha << args.verbose.value() << '\n';
  if (args.count.value().has_value()) {
    std::cout << "count: " << *args.count.value() << '\n';
  }

  std::cout << "input files:\n";
  for (const auto& file : args.input_files.value()) {
    std::cout << "  " << file << '\n';
  }

  if (args.build.provided()) {
    std::cout << "build invoked\n";
    std::cout << "config: " << *args.build.config.value() << '\n';
    std::cout << "release: " << args.build.release.value() << '\n';
  }

  return 0;
}
