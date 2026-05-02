#include <argon/parser.hh>

struct Args {
  struct SubArgs {};
  argon::Command<SubArgs, "sub"> a;
  argon::Command<SubArgs, "sub"> b;  // duplicate subcommand name
};

auto main() -> int {
  auto parser = argon::Parser<Args>{};
}
