#include <argparse/argparse.hpp>

auto main(int argc, char** argv) -> int {
  argparse::ArgumentParser program{"subcommand"};
  argparse::ArgumentParser build{"build"};
  build.add_argument("--profile");
  argparse::ArgumentParser test{"test"};
  test.add_argument("-v", "--verbose").flag();
  program.add_subparser(build);
  program.add_subparser(test);
  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error&) {
    return 1;
  }
  return 0;
}
