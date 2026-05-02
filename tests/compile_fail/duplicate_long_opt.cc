#include <argon/parser.hh>

struct Args {
  argon::Arg<int, "foo", '\0'> a;
  argon::Arg<int, "foo", '\0'> b;  // duplicate --foo
};

auto main() -> int {
  auto parser = argon::Parser<Args>{};
}
