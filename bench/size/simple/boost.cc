#include <boost/program_options.hpp>
#include <string>

namespace po = boost::program_options;

auto main(int argc, char** argv) -> int {
  bool verbose = false;
  std::string output;
  std::string input;
  po::options_description options{"simple"};
  options.add_options()("verbose,v", po::bool_switch(&verbose))(
      "output,o", po::value<std::string>(&output))(
      "input", po::value<std::string>(&input)->required());
  po::positional_options_description positional;
  positional.add("input", 1);
  try {
    po::variables_map values;
    po::store(po::command_line_parser(argc, argv)
                  .options(options)
                  .positional(positional)
                  .run(),
              values);
    po::notify(values);
  } catch (const po::error&) {
    return 1;
  }
  return 0;
}
