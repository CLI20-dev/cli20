#include <argon/parser.hh>

struct Args {
  argon::Arg<int, "foo", 'f'> a;
  argon::Arg<int, "bar", 'f'> b;  // duplicate -f
};

auto main() -> int {
  auto parser = argon::Parser<Args>{};
}
