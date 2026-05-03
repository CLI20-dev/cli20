#include <argon/arithmetic_argument.hh>
#include <argon/flag_argument.hh>
#include <argon/parser.hh>

// Mixing valid argument fields with a plain int must be rejected.
struct Args {
  argon::FlagArg<"verbose"> verbose;
  int count;  // not an argument type
};

auto main() -> int {
  auto parser = argon::Parser<Args>{};
}
