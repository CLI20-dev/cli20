#include <cli/arithmetic_argument.hh>
#include <cli/bool_argument.hh>
#include <cli/flag_argument.hh>
#include <cli/parser.hh>
#include <cli/string_argument.hh>
#include <map>
#include <string>

// Arg<std::map<...>> has no specialization — must trigger the static_assert
// in the primary Arg template with a clear "unsupported value type" message.
struct Args {
  cli::Arg<std::map<std::string, int>, "config", '\0'> cfg;
};

auto main() -> int { auto parser = cli::Parser<Args>{}; }
