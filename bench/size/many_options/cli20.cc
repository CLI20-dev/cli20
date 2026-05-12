#include <cli/cli.hh>

enum class Mode { fast, balanced, precise };

inline auto parse_mode(std::string_view value) -> std::optional<Mode> {
  if (value == "fast") return Mode::fast;
  if (value == "balanced") return Mode::balanced;
  if (value == "precise") return Mode::precise;
  return std::nullopt;
}

struct Args {
  cli::StringOption<"name"> name;
  cli::StringOption<"output"> output;
  cli::StringOption<"target"> target;
  cli::StringOption<"define"> define;
  cli::IntOption<"jobs"> jobs;
  cli::IntOption<"retries"> retries;
  cli::IntOption<"port"> port;
  cli::IntOption<"depth"> depth;
  cli::DoubleOption<"ratio"> ratio;
  cli::DoubleOption<"timeout"> timeout;
  cli::DoubleOption<"threshold"> threshold;
  cli::DoubleOption<"scale"> scale;
  cli::PathOption<"config"> config;
  cli::PathOption<"cache"> cache;
  cli::Arg<"mode",
           cli::conversion::choice<Mode, parse_mode> | cli::pack::set_once>
      mode;
  cli::Arg<"color",
           cli::conversion::choice<Mode, parse_mode> | cli::pack::set_once>
      color;
};

auto main(int argc, char** argv) -> int {
  auto parsed = cli::Parser<Args>{}.parse(argc, argv);
  return parsed.has_value() ? 0 : 1;
}
