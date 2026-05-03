#include <argon/arithmetic_argument.hh>
#include <argon/bool_argument.hh>
#include <argon/flag_argument.hh>
#include <argon/parser.hh>
#include <argon/string_argument.hh>
#include <map>
#include <string>

// Arg<std::map<...>> has no specialization — must trigger the static_assert
// in the primary Arg template with a clear "unsupported value type" message.
struct Args {
  argon::Arg<std::map<std::string, int>, "config", '\0'> cfg;
};

auto main() -> int { auto parser = argon::Parser<Args>{}; }
