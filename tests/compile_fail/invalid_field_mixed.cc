#include <cli/arithmetic_argument.hh>
#include <cli/flag_argument.hh>
#include <cli/parser.hh>

// Mixing valid argument fields with a plain int must be rejected.
struct Args {
  cli::FlagArg<"verbose"> verbose;
  int count;  // not an argument type
};

auto main() -> int { auto parser = cli::Parser<Args>{}; }
