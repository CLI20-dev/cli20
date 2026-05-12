#include <CLI/CLI.hpp>
#include <string>

auto main(int argc, char** argv) -> int {
  CLI::App app{"many_options"};
  std::string values[16];
  app.add_option("--opt01", values[0]);
  app.add_option("--opt02", values[1]);
  app.add_option("--opt03", values[2]);
  app.add_option("--opt04", values[3]);
  app.add_option("--opt05", values[4]);
  app.add_option("--opt06", values[5]);
  app.add_option("--opt07", values[6]);
  app.add_option("--opt08", values[7]);
  app.add_option("--opt09", values[8]);
  app.add_option("--opt10", values[9]);
  app.add_option("--opt11", values[10]);
  app.add_option("--opt12", values[11]);
  app.add_option("--opt13", values[12]);
  app.add_option("--opt14", values[13]);
  app.add_option("--opt15", values[14]);
  app.add_option("--opt16", values[15]);
  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }
  return 0;
}
