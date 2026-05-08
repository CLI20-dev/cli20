// bind.cc — demonstrates BoundOption writing directly into application state.

#include <iostream>

#include "cli/argument.hh"
#include "cli/parser.hh"

struct Args {
  cli::Description description{
      "Bind parsed values into pre-existing variables instead of reading them"
      " back from Args."};
  cli::Help<> help;
  cli::BoundIntOption<"port", 'p'> port;
  cli::BoundStringOption<"output", 'o'> output;
  cli::BoundDoubleOption<"ratio"> ratio;
};

auto main(int argc, char* argv[]) -> int {
  int port = 8080;
  std::string output = "out.bin";
  double ratio = 1.0;

  Args args;
  args.port.bind(port);
  args.output.bind(output);
  args.ratio = &ratio;

  const auto result = cli::Parser<Args>{}.parse(std::move(args), argc, argv);
  if (!result) {
    std::cerr << result.error.message() << '\n';
    return result.error.exit_code();
  }

  std::cout << "port: " << port << '\n';
  std::cout << "output: " << output << '\n';
  std::cout << "ratio: " << ratio << '\n';
  return 0;
}
