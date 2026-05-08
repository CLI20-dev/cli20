// example_bind.cc — demonstrates BoundOption: parsed values written directly
// into user variables instead of being stored inside the Args struct fields.
//
// Run:
//   ./example_bind --port 8080 --output result.bin --ratio 1.5
//   ./example_bind -p 443 -o /tmp/out

#include <iostream>

#include "cli/argument.hh"
#include "cli/parser.hh"

// ── Arg schema ────────────────────────────────────────────────────────────────
//
// BoundXxxOption stores a T* internally.  At parse time it writes through the
// pointer.  The variables that will receive the values are declared BEFORE Args
// is constructed and their addresses are passed to each option.

struct Args {
  cli::Help<> help;
  cli::BoundIntOption<"port", 'p'> port_opt;
  cli::BoundStringOption<"output", 'o'> output_opt;
  cli::BoundDoubleOption<"ratio"> ratio_opt;
};

auto main(int argc, char* argv[]) -> int {
  // Declare target variables with sensible defaults.
  int port = 8080;
  std::string output = "out.bin";
  double ratio = 1.0;

  // Bind each option to its target variable.
  Args args;
  args.port_opt.bind(port);
  args.output_opt.bind(output);
  args.ratio_opt = &ratio;  // operator=(T*) also works

  // Parse — pass the pre-initialized args to preserve the pointer bindings.
  auto result = cli::Parser<Args>{}.parse(std::move(args), argc, argv);

  if (!result) {
    std::cerr << result.error.message() << '\n';
    return result.error.exit_code();
  }

  // Values are now in the original local variables — no .value() needed.
  std::cout << "port:   " << port << '\n';
  std::cout << "output: " << output << '\n';
  std::cout << "ratio:  " << ratio << '\n';

  return 0;
}
