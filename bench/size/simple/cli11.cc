#include <CLI/CLI.hpp>
#include <string>

auto main(int argc, char** argv) -> int {
  CLI::App app{"simple"};
  bool verbose = false;
  std::string output;
  std::string input;
  app.add_flag("-v,--verbose", verbose);
  app.add_option("-o,--output", output);
  app.add_option("input", input)->required();
  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }
  return 0;
}
