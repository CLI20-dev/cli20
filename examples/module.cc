#include <iostream>
#include <string>

import cli20;

using std::string;

using cli::Description;
using cli::Help;
using cli::IntOption;
using cli::Positional;
using cli::Presence;

using namespace cli::nargs;

struct Args {
  Description description{"C++20 module import example for cli20."};
  Help<> help{{.help = "Show help", .presence = Presence::optional}};
  IntOption<"port", 'p'> port{{.help = "TCP port number"}};
  Positional<string, one_or_more> files{
      {.help = "One or more input files", .presence = Presence::required}};
};

auto main(int argc, char* argv[]) -> int {
  const Args args = cli::parse_or_exit<Args>(argc, argv);
  if (args.port.value()) {
    std::cout << "port: " << *args.port.value() << '\n';
  }
  for (const string& file : args.files.value()) {
    std::cout << "file: " << file << '\n';
  }
  return 0;
}
