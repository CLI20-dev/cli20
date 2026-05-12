#pragma once

#include <filesystem>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

namespace bench {

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

struct ManyValues {
  std::string name;
  std::string output;
  std::string target;
  std::string define;
  int jobs{};
  int retries{};
  int port{};
  int depth{};
  double ratio{};
  double timeout{};
  double threshold{};
  double scale{};
  std::filesystem::path config;
  std::filesystem::path cache;
  Mode mode{};
  Mode color{};
};

}  // namespace bench
