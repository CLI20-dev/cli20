#include <cxxopts.hpp>

enum class Mode { fast, balanced, precise };

inline auto mode_name(Mode mode) -> std::string_view {
  switch (mode) {
    case Mode::fast:
      return "fast";
    case Mode::balanced:
      return "balanced";
    case Mode::precise:
      return "precise";
  }
  return "unknown";
}

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

inline auto operator>>(std::istream& in, Mode& mode) -> std::istream& {
  std::string value;
  in >> value;
  if (auto parsed = parse_mode(value)) {
    mode = *parsed;
  } else {
    in.setstate(std::ios::failbit);
  }
  return in;
}

inline auto operator<<(std::ostream& out, Mode mode) -> std::ostream& {
  return out << mode_name(mode);
}

auto main(int argc, char** argv) -> int {
  cxxopts::Options options{"many_options"};
  options.add_options()("name", "name", cxxopts::value<std::string>())(
      "output", "output", cxxopts::value<std::string>())(
      "target", "target", cxxopts::value<std::string>())(
      "define", "define", cxxopts::value<std::string>())("jobs", "jobs",
                                                         cxxopts::value<int>())(
      "retries", "retries", cxxopts::value<int>())("port", "port",
                                                   cxxopts::value<int>())(
      "depth", "depth", cxxopts::value<int>())("ratio", "ratio",
                                               cxxopts::value<double>())(
      "timeout", "timeout", cxxopts::value<double>())("threshold", "threshold",
                                                      cxxopts::value<double>())(
      "scale", "scale", cxxopts::value<double>())(
      "config", "config", cxxopts::value<std::filesystem::path>())(
      "cache", "cache", cxxopts::value<std::filesystem::path>())(
      "mode", "mode", cxxopts::value<Mode>())("color", "color",
                                              cxxopts::value<Mode>());
  try {
    (void)options.parse(argc, argv);
  } catch (const cxxopts::exceptions::exception&) {
    return 1;
  }
  return 0;
}
