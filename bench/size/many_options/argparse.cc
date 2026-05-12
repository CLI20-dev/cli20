#include <argparse/argparse.hpp>

enum class Mode { fast, balanced, precise };

inline auto parse_mode(std::string_view value) -> std::optional<Mode> {
  if (value == "fast") return Mode::fast;
  if (value == "balanced") return Mode::balanced;
  if (value == "precise") return Mode::precise;
  return std::nullopt;
}

inline auto parse_mode_or_throw(const std::string& value) -> Mode {
  if (auto mode = parse_mode(value)) return *mode;
  throw std::runtime_error("invalid mode: " + value);
}

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
  program.add_argument("--config")
      .action([](const std::string& value) -> std::filesystem::path {
        return std::filesystem::path{value};
      });
  program.add_argument("--cache").action(
      [](const std::string& value) -> std::filesystem::path {
        return std::filesystem::path{value};
      });
  program.add_argument("--mode").action(parse_mode_or_throw);
  program.add_argument("--color").action(parse_mode_or_throw);
  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error&) {
    return 1;
  }
  return 0;
}
