#include <iostream>

#include "argon/argument.hh"
#include "argon/parser.hh"

struct Args {
  argon::Description description{
      "Small example used by the README and by CI to exercise the public API."};

  argon::HelpFlag<>
      help{{.help = "Show help", .presence = argon::Presence::optional}};

  argon::IntOption<"port", 'p'>
      port{{.help = "TCP port number", .presence = argon::Presence::optional}};

  argon::Positional<std::string, argon::nargs::one_or_more>
      files{{.help = "One or more input files",
             .presence = argon::Presence::required}};
};

auto main(int argc, char* argv[]) -> int {
  const auto args = argon::parseOrExit<Args>(argc, argv);
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
