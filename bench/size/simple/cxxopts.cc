#include <cxxopts.hpp>
#include <string>

auto main(int argc, char** argv) -> int {
  cxxopts::Options options{"simple"};
  options.add_options()("v,verbose", "verbose",
                        cxxopts::value<bool>()->default_value("false"))(
      "o,output", "output", cxxopts::value<std::string>())(
      "input", "input", cxxopts::value<std::string>());
  options.parse_positional({"input"});
  try {
    (void)options.parse(argc, argv);
  } catch (const cxxopts::exceptions::exception&) {
    return 1;
  }
  return 0;
}
