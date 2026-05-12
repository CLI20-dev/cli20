#include "cli/argument.hh"
#include "cli/parser.hh"

struct BuildArgs {
  cli::StringOption<"profile"> profile;
};

struct TestArgs {
  cli::Flag<"verbose", 'v'> verbose;
};

struct Args {
  cli::Command<"build", BuildArgs> build;
  cli::Command<"test", TestArgs> test;
};

auto main(int argc, char** argv) -> int {
  auto parsed = cli::Parser<Args>{}.parse(argc, argv);
  return parsed.has_value() ? 0 : 1;
}
