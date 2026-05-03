#include <argon/flag_argument.hh>
#include <argon/parser.hh>

// Plain int field is not an argument type — Parser<T> must reject this.
struct Args {
  int x;
};

auto main() -> int { auto parser = argon::Parser<Args>{}; }
