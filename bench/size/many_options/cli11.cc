#include <CLI/CLI.hpp>
#include <map>

#include "bench/many_options.hh"

auto main(int argc, char** argv) -> int {
  CLI::App app{"many_options"};
  bench::ManyValues values;
  static const std::map<std::string, bench::Mode> modes{
      {"fast", bench::Mode::fast},
      {"balanced", bench::Mode::balanced},
      {"precise", bench::Mode::precise},
  };
  app.add_option("--name", values.name);
  app.add_option("--output", values.output);
  app.add_option("--target", values.target);
  app.add_option("--define", values.define);
  app.add_option("--jobs", values.jobs);
  app.add_option("--retries", values.retries);
  app.add_option("--port", values.port);
  app.add_option("--depth", values.depth);
  app.add_option("--ratio", values.ratio);
  app.add_option("--timeout", values.timeout);
  app.add_option("--threshold", values.threshold);
  app.add_option("--scale", values.scale);
  app.add_option("--config", values.config);
  app.add_option("--cache", values.cache);
  app.add_option("--mode", values.mode)
      ->transform(CLI::CheckedTransformer(modes));
  app.add_option("--color", values.color)
      ->transform(CLI::CheckedTransformer(modes));
  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }
  return 0;
}
