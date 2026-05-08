#include <iostream>

#include "cli/argument.hh"
#include "cli/parser.hh"

struct Args {
  cli::Description description{
      "Minimal cli20 program with one flag, one option, and one positional."};
  cli::Help<> help;
  cli::Flag<"verbose", 'v'> verbose{{.help = "Enable verbose logging"}};
  cli::StringOption<"output", 'o'> output{{.help = "Optional output file"}};
  cli::Positional<std::string> input{
      {.help = "Input file", .presence = cli::required}};
};

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parse_or_exit<Args>(argc, argv);

  if (args.verbose.value()) {
    std::cout << "verbose: true\n";
  }
  if (args.output.value()) {
    std::cout << "output: " << *args.output.value() << '\n';
  }
  std::cout << "input: " << *args.input.value() << '\n';
  return 0;
}
