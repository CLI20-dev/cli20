#include <iostream>

#include "cli20.hh"

struct Args {
  cli::Description description{"Single-header distribution example."};
  cli::Help<> help;
  cli::Flag<"verbose", 'v'> verbose{{.help = "Enable verbose mode"}};
  cli::StringOption<"config", 'c'> config{{.help = "Configuration name"}};
  cli::Positional<std::string> input{
      {.help = "Input file", .presence = cli::required}};
};

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parse_or_exit<Args>(argc, argv);
  std::cout << "verbose: " << std::boolalpha << args.verbose.value() << '\n';
  if (args.config) {
    std::cout << "config: " << *args.config << '\n';
  }
  std::cout << "input: " << *args.input << '\n';
  return 0;
}
