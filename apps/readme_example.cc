#include <iostream>

#include "cli/argument.hh"
#include "cli/parser.hh"

struct Args {
  cli::Description description{
      "Small example used by the README and by CI to exercise the public API."};

  cli::Help<> help{{.help = "Show help", .presence = cli::Presence::optional}};

  cli::IntOption<"port", 'p'> port{
      {.help = "TCP port number", .presence = cli::Presence::optional}};

  cli::Positional<std::string, cli::nargs::one_or_more> files{
      {.help = "One or more input files", .presence = cli::Presence::required}};
};

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parseOrExit<Args>(argc, argv);
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
