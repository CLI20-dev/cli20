#include <cli/flag_argument.hh>
#include <cli/parser.hh>

// Passing an argument type itself (not a struct of arguments) must be rejected.
// FlagArg derives from ArgumentTag, so isValidArgumentsType returns false.
auto main() -> int { auto parser = cli::Parser<cli::FlagArg<"verbose">>{}; }
