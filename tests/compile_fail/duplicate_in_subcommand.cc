#include <cli/parser.hh>

struct Args {
  struct SubArgs {
    cli::Arg<int, "foo", '\0'> a;
    cli::Arg<int, "foo", '\0'> b;  // duplicate --foo inside subcommand
  };
  cli::Command<SubArgs, "sub"> sub;
};

auto main() -> int { auto parser = cli::Parser<Args>{}; }
