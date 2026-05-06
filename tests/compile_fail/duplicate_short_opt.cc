#include <cli/parser.hh>

struct Args {
  cli::Arg<int, "foo", 'f'> a;
  cli::Arg<int, "bar", 'f'> b;  // duplicate -f
};

auto main() -> int { auto parser = cli::Parser<Args>{}; }
