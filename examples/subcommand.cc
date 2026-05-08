#include <iostream>
#include <string>

#include "cli/argument.hh"
#include "cli/parser.hh"

struct BuildArgs {
  cli::Description description{"Build source files into an artifact."};
  cli::Help<> help;
  cli::Flag<"release", 'r'> release{{.help = "Enable optimizations"}};
  cli::IntOption<"jobs", 'j'> jobs{{.help = "Parallel job count"}};
  cli::Positional<std::string, cli::nargs::one_or_more> inputs{
      {.help = "Source files", .presence = cli::required}};
};

struct PublishArgs {
  cli::Description description{"Publish an already-built artifact."};
  cli::Help<> help;
  cli::StringOption<"registry", 'r'> registry{
      {.help = "Destination registry", .presence = cli::required}};
  cli::Flag<"dry-run"> dry_run{{.help = "Print the action without pushing"}};
};

struct Args {
  cli::Description description{
      "Subcommand-oriented example with nested help and program naming."};
  cli::Help<> help;
  cli::Flag<"verbose", 'v'> verbose{{.help = "Enable verbose logging"}};
  cli::Command<"build", BuildArgs> build{{.help = "Build artifacts"}};
  cli::Command<"publish", PublishArgs> publish{{.help = "Publish artifacts"}};
};

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parse_or_exit<Args>(argc, argv);

  std::cout << "verbose: " << std::boolalpha << args.verbose.value() << '\n';

  if (args.build.provided()) {
    std::cout << "command: build\n";
    if (args.build.release.value()) {
      std::cout << "release: true\n";
    }
    if (args.build.jobs.value()) {
      std::cout << "jobs: " << *args.build.jobs.value() << '\n';
    }
    for (const std::string& input : args.build.inputs.value()) {
      std::cout << "input: " << input << '\n';
    }
  }

  if (args.publish.provided()) {
    std::cout << "command: publish\n";
    std::cout << "registry: " << *args.publish.registry.value() << '\n';
    std::cout << "dry-run: " << std::boolalpha << args.publish.dry_run.value()
              << '\n';
  }

  return 0;
}
