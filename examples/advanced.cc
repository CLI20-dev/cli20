#include <filesystem>
#include <iostream>

#include "cli/argument.hh"
#include "cli/parser.hh"

namespace fs = std::filesystem;

struct Args {
  cli::Description description{
      "Action-pipeline example using Arg, filesystem conversion, and"
      " collection-valued options."};

  cli::Arg<"help", 'h', cli::action::print_help | cli::action::exit_success,
           cli::nargs::none>
      help{{.help = "Show help and exit"}};

  cli::Arg<"config", 'c',
           cli::conversion::path | cli::validation::exists |
               cli::validation::is_regular_file | cli::pack::set_once>
      config{{.help = "Configuration file", .presence = cli::required}};

  cli::Arg<"jobs", 'j',
           cli::conversion::integer<int> | cli::validation::range<1, 64> |
               cli::pack::set_once>
      jobs{{.help = "Parallel job count (1-64)"}};

  cli::Arg<"include", 'I',
           cli::conversion::path | cli::validation::parent_exists |
               cli::pack::push,
           cli::nargs::one_or_more>
      includes{{.help = "Include directories to add"}};

  cli::Arg<"define", 'D',
           cli::conversion::string | cli::validation::not_blank |
               cli::pack::push_unique,
           cli::nargs::one_or_more>
      defines{{.help = "Unique preprocessor definitions"}};
};

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parse_or_exit<Args>(argc, argv);

  std::cout << "config: " << args.config.value()->string() << '\n';
  if (args.jobs.value()) {
    std::cout << "jobs: " << *args.jobs.value() << '\n';
  }
  for (const fs::path& include_dir : args.includes.value()) {
    std::cout << "include: " << include_dir.string() << '\n';
  }
  for (const std::string& define : args.defines.value()) {
    std::cout << "define: " << define << '\n';
  }
  return 0;
}
