#include <cxxopts.hpp>

#include "bench/many_options.hh"

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
      "mode", "mode", cxxopts::value<bench::Mode>())(
      "color", "color", cxxopts::value<bench::Mode>());
  try {
    (void)options.parse(argc, argv);
  } catch (const cxxopts::exceptions::exception&) {
    return 1;
  }
  return 0;
}
