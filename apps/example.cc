#include <filesystem>
#include <iostream>

#include "argon/argument.hh"
#include "argon/parser.hh"

namespace fs = std::filesystem;

struct BuildArgs {
  // `ArgImpl` remains available when you need validation beyond the sugar aliases.
  argon::ArgImpl<
      "config", 'c', argon::nargs::one,
      argon::Action<argon::conversion::path, argon::validation::exists,
                    argon::validation::is_regular_file, argon::pack::set_once>{}>
      config{{.help = "Build configuration file",
              .presence = argon::Presence::required}};

  argon::FlagOption<"release", 'r'>
      release{{.help = "Build with release optimizations",
               .presence = argon::Presence::optional}};
};

struct Args {
  argon::Description description{
      "Demonstrates the current argon parser API with options, positionals, and "
      "a subcommand."};

  argon::ArgImpl<"help", 'h', argon::nargs::none,
                 argon::Action<argon::action::print_help,
                               argon::action::exit_success>{}>
      help{{.help = "Show this help message and exit",
            .presence = argon::Presence::optional}};

  argon::FlagOption<"verbose", 'v'>
      verbose{{.help = "Enable verbose output",
               .presence = argon::Presence::optional}};

  argon::IntOption<"count", 'n'>
      count{{.help = "Positive iteration count",
             .presence = argon::Presence::optional}};

  argon::Positional<std::string, argon::nargs::one_or_more>
      input_files{
          {.help = "Input files", .presence = argon::Presence::required}};

  argon::Command<"build", BuildArgs> build{{.help = "Run the build subcommand"}};
};

auto main(int argc, char* argv[]) -> int {
  const auto args = argon::parseOrExit<Args>(argc, argv);
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
