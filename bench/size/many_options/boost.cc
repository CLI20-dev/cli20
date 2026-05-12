#include <boost/program_options.hpp>
#include <boost/program_options/errors.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace po = boost::program_options;

enum class Mode { fast, balanced, precise };

inline auto parse_mode(std::string_view value) -> std::optional<Mode> {
  if (value == "fast") return Mode::fast;
  if (value == "balanced") return Mode::balanced;
  if (value == "precise") return Mode::precise;
  return std::nullopt;
}

inline void validate(boost::any& out, const std::vector<std::string>& values,
                     Mode*, int) {
  po::validators::check_first_occurrence(out);
  const auto& value = po::validators::get_single_string(values);
  if (auto mode = parse_mode(value)) {
    out = *mode;
    return;
  }
  throw po::validation_error(po::validation_error::invalid_option_value, value);
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

auto main(int argc, char** argv) -> int {
  po::options_description options{"many_options"};
  ManyValues values;
  options.add_options()("name", po::value<std::string>(&values.name))(
      "output", po::value<std::string>(&values.output))(
      "target", po::value<std::string>(&values.target))(
      "define", po::value<std::string>(&values.define))(
      "jobs", po::value<int>(&values.jobs))(
      "retries", po::value<int>(&values.retries))("port",
                                                  po::value<int>(&values.port))(
      "depth", po::value<int>(&values.depth))("ratio",
                                              po::value<double>(&values.ratio))(
      "timeout", po::value<double>(&values.timeout))(
      "threshold", po::value<double>(&values.threshold))(
      "scale", po::value<double>(&values.scale))(
      "config", po::value<std::filesystem::path>(&values.config))(
      "cache", po::value<std::filesystem::path>(&values.cache))(
      "mode", po::value<Mode>(&values.mode))("color",
                                             po::value<Mode>(&values.color));
  try {
    po::variables_map parsed;
    po::store(po::command_line_parser(argc, argv).options(options).run(),
              parsed);
    po::notify(parsed);
  } catch (const po::error&) {
    return 1;
  }
  return 0;
}
