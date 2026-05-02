#include <iostream>

#include "argon/argument.hh"
#include "argon/int_argument.hh"
#include "argon/parser.hh"

struct Args {
  argon::Flag<"flag1", 'a'> flag1;

  argon::IntArg<"arg1", 'b'> arg1;
  argon::IntArg<"arg2"> arg2;
  argon::IntArg<"arg3"> arg3;
  argon::IntListArg<"arg4", 'c'> arg4{argon::nargs::one_or_more};

  argon::PositionalArgument<std::string> positional1;

  struct SubArgs {
    argon::IntArg<"arg1", 'a'> arg1;
    argon::IntArg<"arg2", 'b'> arg2;
  };
  argon::Command<SubArgs, "subcommand"> subcommand;
};

auto main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) -> int {
  auto parser = argon::Parser<Args>{};

  auto res = parser.parse(argc, argv);

  if (!res) {
    std::cerr << "Error parsing arguments: " << res.error() << std::endl;
    return 1;
  }

  return 0;
}
