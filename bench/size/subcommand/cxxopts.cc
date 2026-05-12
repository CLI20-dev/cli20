#include <cxxopts.hpp>
#include <string>

auto main(int argc, char** argv) -> int {
  if (argc < 2) return 1;
  const std::string command = argv[1];
  try {
    if (command == "build") {
      cxxopts::Options options{"build"};
      options.add_options()("profile", "profile", cxxopts::value<std::string>());
      (void)options.parse(argc - 1, argv + 1);
      return 0;
    }
    if (command == "test") {
      cxxopts::Options options{"test"};
      options.add_options()("v,verbose", "verbose",
                            cxxopts::value<bool>()->default_value("false"));
      (void)options.parse(argc - 1, argv + 1);
      return 0;
    }
  } catch (const cxxopts::exceptions::exception&) {
    return 1;
  }
  return 1;
}
