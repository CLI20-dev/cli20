#include <filesystem>
#include <iostream>

#include "cli/argument.hh"
#include "cli/parser.hh"

namespace fs = std::filesystem;

struct BuildArgs {
  // `ArgImpl` remains available when you need validation beyond the sugar
  // aliases.
  cli::ArgImpl<
      "config", 'c', cli::nargs::one,
      cli::Action<cli::conversion::path, cli::validation::exists,
                  cli::validation::is_regular_file, cli::pack::set_once>{}>
      config{{.help = "Build configuration file",
              .presence = cli::Presence::required}};

  cli::Flag<"release", 'r'> release{{.help = "Build with release optimizations",
                                     .presence = cli::Presence::optional}};
};

struct Args {
  cli::Description description{
      "Demonstrates the current cli20 parser API with options, positionals, and "
      "a subcommand."};

  cli::ArgImpl<"help", 'h', cli::nargs::none,
               cli::Action<cli::action::print_help, cli::action::exit_success>{}>
      help{{.help = "Show this help message and exit",
            .presence = cli::Presence::optional}};

  cli::Flag<"verbose", 'v'> verbose{
      {.help = "Enable verbose output", .presence = cli::Presence::optional}};

  cli::IntOption<"count", 'n'> count{
      {.help = "Positive iteration count", .presence = cli::Presence::optional}};

  cli::Positional<std::string, cli::nargs::one_or_more> input_files{
      {.help = "Input files", .presence = cli::Presence::required}};

  cli::Command<"build", BuildArgs> build{{.help = "Run the build subcommand"}};
};

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parseOrExit<Args>(argc, argv);
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
    std::cout << "config: " << args.build.config.value()->string() << '\n';
    std::cout << "release: " << args.build.release.value() << '\n';
  }

  return 0;
}
