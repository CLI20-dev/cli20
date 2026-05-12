#include <argparse/argparse.hpp>

#include "bench/many_options.hh"

auto main(int argc, char** argv) -> int {
  argparse::ArgumentParser program{"many_options"};
  program.add_argument("--name");
  program.add_argument("--output");
  program.add_argument("--target");
  program.add_argument("--define");
  program.add_argument("--jobs").scan<'i', int>();
  program.add_argument("--retries").scan<'i', int>();
  program.add_argument("--port").scan<'i', int>();
  program.add_argument("--depth").scan<'i', int>();
  program.add_argument("--ratio").scan<'g', double>();
  program.add_argument("--timeout").scan<'g', double>();
  program.add_argument("--threshold").scan<'g', double>();
  program.add_argument("--scale").scan<'g', double>();
  program.add_argument("--config").action([](const std::string& value) {
    return std::filesystem::path{value};
  });
  program.add_argument("--cache").action(
      [](const std::string& value) { return std::filesystem::path{value}; });
  program.add_argument("--mode").action(bench::parse_mode_or_throw);
  program.add_argument("--color").action(bench::parse_mode_or_throw);
  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error&) {
    return 1;
  }
  return 0;
}
