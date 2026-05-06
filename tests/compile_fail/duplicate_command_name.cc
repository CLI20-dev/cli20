#include <cli/parser.hh>

struct Args {
  struct SubArgs {};
  cli::Command<SubArgs, "sub"> a;
  cli::Command<SubArgs, "sub"> b;  // duplicate subcommand name
};

auto main() -> int { auto parser = cli::Parser<Args>{}; }
