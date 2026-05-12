#include "cli/argument.hh"
#include "cli/parser.hh"

struct Args {
  cli::StringOption<"opt01"> opt01;
  cli::StringOption<"opt02"> opt02;
  cli::StringOption<"opt03"> opt03;
  cli::StringOption<"opt04"> opt04;
  cli::StringOption<"opt05"> opt05;
  cli::StringOption<"opt06"> opt06;
  cli::StringOption<"opt07"> opt07;
  cli::StringOption<"opt08"> opt08;
  cli::StringOption<"opt09"> opt09;
  cli::StringOption<"opt10"> opt10;
  cli::StringOption<"opt11"> opt11;
  cli::StringOption<"opt12"> opt12;
  cli::StringOption<"opt13"> opt13;
  cli::StringOption<"opt14"> opt14;
  cli::StringOption<"opt15"> opt15;
  cli::StringOption<"opt16"> opt16;
};

auto main(int argc, char** argv) -> int {
  auto parsed = cli::Parser<Args>{}.parse(argc, argv);
  return parsed.has_value() ? 0 : 1;
}
