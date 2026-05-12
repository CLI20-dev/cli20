#include <boost/program_options.hpp>
#include <string>

namespace po = boost::program_options;

auto main(int argc, char** argv) -> int {
  if (argc < 2) return 1;
  const std::string command = argv[1];
  try {
    if (command == "build") {
      std::string profile;
      po::options_description options{"build"};
      options.add_options()("profile", po::value<std::string>(&profile));
      po::variables_map values;
      po::store(
          po::command_line_parser(argc - 1, argv + 1).options(options).run(),
          values);
      po::notify(values);
      return 0;
    }
    if (command == "test") {
      bool verbose = false;
      po::options_description options{"test"};
      options.add_options()("verbose,v", po::bool_switch(&verbose));
      po::variables_map values;
      po::store(
          po::command_line_parser(argc - 1, argv + 1).options(options).run(),
          values);
      po::notify(values);
      return 0;
    }
  } catch (const po::error&) {
    return 1;
  }
  return 1;
}
