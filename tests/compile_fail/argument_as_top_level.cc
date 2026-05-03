#include <argon/flag_argument.hh>
#include <argon/parser.hh>

// Passing an argument type itself (not a struct of arguments) must be rejected.
// FlagArg derives from ArgumentTag, so isValidArgumentsType returns false.
auto main() -> int {
  auto parser = argon::Parser<argon::FlagArg<"verbose">>{};
}
