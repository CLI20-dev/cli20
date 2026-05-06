#include <filesystem>
#include <iostream>

#include "argon/argument.hh"
#include "argon/parser.hh"

namespace fs = std::filesystem;

struct BuildArgs {
  argon::ArgImpl<
      "config", 'c', {.min = 1, .max = 1},
      argon::Action<argon::conversion::path, argon::validation::exists,
                    argon::validation::is_regular_file, argon::pack::set_once>{}>
      config{{.help = "Build configuration file",
              .presence = argon::Presence::required}};

  argon::ArgImpl<"release", 'r', {.min = 0, .max = 0},
                 argon::Action<argon::pack::set_true>{}>
      release{{.help = "Build with release optimizations",
               .presence = argon::Presence::optional}};
};

struct Args {
  argon::Description description{
      "Demonstrates the current argon parser API with options, positionals, and "
      "a subcommand."};

  argon::ArgImpl<"verbose", 'v', {.min = 0, .max = 0},
                 argon::Action<argon::pack::set_true>{}>
      verbose{{.help = "Enable verbose output",
               .presence = argon::Presence::optional}};

  argon::ArgImpl<
      "count", 'n', {.min = 1, .max = 1},
      argon::Action<argon::conversion::integer<int>, argon::validation::positive,
                    argon::pack::set_once>{}>
      count{{.help = "Positive iteration count",
             .presence = argon::Presence::optional}};

  argon::PositionalImpl<
      {.min = 1, .max = -1},
      argon::Action<argon::conversion::string, argon::validation::not_blank,
                    argon::pack::push>{}>
      input_files{
          {.help = "Input files", .presence = argon::Presence::required}};

  argon::Command<"build", BuildArgs> build{{.help = "Run the build subcommand"}};
};

auto main(int argc, char* argv[]) -> int {
  auto result = argon::parse<Args>(argc, argv);
  if (!result) {
    std::cerr << result.error.message() << '\n';
    return 1;
  }

  const auto& args = result.value;
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
