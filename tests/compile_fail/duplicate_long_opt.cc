#include <cli/parser.hh>

struct Args {
  cli::Arg<int, "foo", '\0'> a;
  cli::Arg<int, "foo", '\0'> b;  // duplicate --foo
};

auto main() -> int { auto parser = cli::Parser<Args>{}; }
