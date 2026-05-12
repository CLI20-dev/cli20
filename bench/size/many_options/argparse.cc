#include <argparse/argparse.hpp>

auto main(int argc, char** argv) -> int {
  argparse::ArgumentParser program{"many_options"};
  program.add_argument("--opt01");
  program.add_argument("--opt02");
  program.add_argument("--opt03");
  program.add_argument("--opt04");
  program.add_argument("--opt05");
  program.add_argument("--opt06");
  program.add_argument("--opt07");
  program.add_argument("--opt08");
  program.add_argument("--opt09");
  program.add_argument("--opt10");
  program.add_argument("--opt11");
  program.add_argument("--opt12");
  program.add_argument("--opt13");
  program.add_argument("--opt14");
  program.add_argument("--opt15");
  program.add_argument("--opt16");
  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error&) {
    return 1;
  }
  return 0;
}
