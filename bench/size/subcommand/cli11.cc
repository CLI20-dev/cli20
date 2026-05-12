#include <CLI/CLI.hpp>
#include <string>

auto main(int argc, char** argv) -> int {
  CLI::App app{"subcommand"};
  std::string profile;
  bool verbose = false;
  auto* build = app.add_subcommand("build");
  build->add_option("--profile", profile);
  auto* test = app.add_subcommand("test");
  test->add_flag("-v,--verbose", verbose);
  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }
  return 0;
}
