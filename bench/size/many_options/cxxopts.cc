#include <cxxopts.hpp>
#include <string>

auto main(int argc, char** argv) -> int {
  cxxopts::Options options{"many_options"};
  options.add_options()("opt01", "opt01", cxxopts::value<std::string>())(
      "opt02", "opt02", cxxopts::value<std::string>())(
      "opt03", "opt03", cxxopts::value<std::string>())(
      "opt04", "opt04", cxxopts::value<std::string>())(
      "opt05", "opt05", cxxopts::value<std::string>())(
      "opt06", "opt06", cxxopts::value<std::string>())(
      "opt07", "opt07", cxxopts::value<std::string>())(
      "opt08", "opt08", cxxopts::value<std::string>())(
      "opt09", "opt09", cxxopts::value<std::string>())(
      "opt10", "opt10", cxxopts::value<std::string>())(
      "opt11", "opt11", cxxopts::value<std::string>())(
      "opt12", "opt12", cxxopts::value<std::string>())(
      "opt13", "opt13", cxxopts::value<std::string>())(
      "opt14", "opt14", cxxopts::value<std::string>())(
      "opt15", "opt15", cxxopts::value<std::string>())(
      "opt16", "opt16", cxxopts::value<std::string>());
  try {
    (void)options.parse(argc, argv);
  } catch (const cxxopts::exceptions::exception&) {
    return 1;
  }
  return 0;
}
