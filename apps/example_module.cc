#include <iostream>
#include <string>

import cli20;

using std::string;

using cli::Description;
using cli::Flag;
using cli::Help;
using cli::IntOption;
using cli::Positional;
using cli::Presence;
using cli::StringOption;

using namespace cli::nargs;

struct Args {
  Description description{
      "Small example used by the README and by CI to exercise the public API."};
  Help<> help{{.help = "Show help", .presence = Presence::optional}};

  IntOption<"port", 'p'> port{
      {.help = "TCP port number", .presence = Presence::optional}};

  Positional<string, one_or_more> files{
      {.help = "One or more input files", .presence = Presence::required}};
};

int main(int argc, char* argv[]) {
  const Args args = cli::parseOrExit<Args>(argc, argv);
  if (args.port.value().has_value()) {
    std::cout << "port: " << *args.port.value() << '\n';
  }

  std::cout << "files:";
  for (const string& file : args.files.value()) {
    std::cout << ' ' << file;
  }
  std::cout << '\n';

  return 0;
}
