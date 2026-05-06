#include <iostream>

#include "argon/argument.hh"
#include "argon/parser.hh"

struct Args {
  argon::Description description{
      "Small example used by the README and by CI to exercise the public API."};

  argon::ArgImpl<"help", 'h', {.min = 0, .max = 0},
                 argon::Action<argon::pack::set_true>{}>
      help{{.help = "Show help", .presence = argon::Presence::optional}};

  argon::ArgImpl<
      "port", 'p', {.min = 1, .max = 1},
      argon::Action<argon::conversion::integer<int>, argon::validation::min<1>,
                    argon::validation::max<65535>, argon::pack::set_once>{}>
      port{{.help = "TCP port number", .presence = argon::Presence::optional}};

  argon::PositionalImpl<
      {.min = 1, .max = -1},
      argon::Action<argon::conversion::string, argon::validation::not_blank,
                    argon::pack::push>{}>
      files{{.help = "One or more input files",
             .presence = argon::Presence::required}};
};

auto main(int argc, char* argv[]) -> int {
  auto parsed = argon::parse<Args>(argc, argv);
  if (!parsed) {
    std::cerr << parsed.error.message() << '\n';
    return 1;
  }

  const auto& args = parsed.value;
  std::cout << "help: " << std::boolalpha << args.help.value() << '\n';
  if (args.port.value().has_value()) {
    std::cout << "port: " << *args.port.value() << '\n';
  }

  std::cout << "files:";
  for (const auto& file : args.files.value()) {
    std::cout << ' ' << file;
  }
  std::cout << '\n';

  return 0;
}
