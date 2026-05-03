#include <argon/parser.hh>

struct Args {
  struct SubArgs {
    argon::Arg<int, "foo", '\0'> a;
    argon::Arg<int, "foo", '\0'> b;  // duplicate --foo inside subcommand
  };
  argon::Command<SubArgs, "sub"> sub;
};

auto main() -> int { auto parser = argon::Parser<Args>{}; }
