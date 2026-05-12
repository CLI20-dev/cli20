#include <string>

#include "cli/argument.hh"
#include "cli/parser.hh"

struct Args {
  cli::Flag<"verbose", 'v'> verbose;
  cli::StringOption<"output", 'o'> output;
  cli::Positional<std::string> input{{.presence = cli::required}};
};

auto main(int argc, char** argv) -> int {
  auto parsed = cli::Parser<Args>{}.parse(argc, argv);
  return parsed.has_value() ? 0 : 1;
}
