#include <argparse/argparse.hpp>

auto main(int argc, char** argv) -> int {
  argparse::ArgumentParser program{"simple"};
  program.add_argument("-v", "--verbose").flag();
  program.add_argument("-o", "--output");
  program.add_argument("input");
  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error&) {
    return 1;
  }
  return 0;
}
