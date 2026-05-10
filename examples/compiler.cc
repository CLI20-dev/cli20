#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "cli/argument.hh"
#include "cli/parser.hh"

struct Args {
  cli::Description description{
      "Compiler-wrapper example: short-cluster flags, -W<warning>, "
      "--feature=<name> with negatable enable/disable."};

  // cli::Help<> is sugar for Arg<"help",'h', print_help|exit_success, nargs::none>
  cli::Help<> help{{.help = "Show help and exit"}};

  // -O0 .. -O3
  cli::Arg<"optimize", 'O', cli::conversion::integer<int> |
                                 cli::validation::range<0, 3> |
                                 cli::pack::set_once>
      optimize{{.help = "Optimisation level (0-3)"}};

  // -v / -vv: counted verbosity
  cli::Arg<"verbose", 'v', cli::pack::increment, cli::nargs::none>
      verbose{{.help = "Increase verbosity (repeatable)"}};

  // -Werror      → warnings["error"]  = true
  // -Wno-unused  → warnings["unused"] = false
  cli::Arg<"warning", 'W',
           cli::conversion::negatable<"no-"> | cli::pack::insert_or_assign>
      warnings{{.help = "Enable/disable a warning (e.g. -Werror, -Wno-unused)"}};

  // --feature=lto    → features["lto"]  = true
  // --feature=no-pgo → features["pgo"]  = false
  cli::Arg<"feature", '\0',
           cli::conversion::negatable<"no-"> | cli::pack::insert_or_assign>
      features{{.help = "Enable/disable a feature (e.g. --feature=lto, "
                        "--feature=no-pgo)"}};

  cli::Positional<std::string, cli::nargs::one_or_more> sources{
      {.help = "Source files", .presence = cli::required}};
};

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parse_or_exit<Args>(argc, argv);

  if (args.optimize.value()) {
    std::cout << "optimize: -O" << *args.optimize.value() << '\n';
  }

  if (const std::size_t v = args.verbose.value(); v > 0) {
    std::cout << "verbosity: " << v << '\n';
  }

  for (const auto& [name, enabled] : args.warnings.value()) {
    std::cout << "warning: " << name << " = " << (enabled ? "true" : "false")
              << '\n';
  }

  for (const auto& [name, enabled] : args.features.value()) {
    std::cout << "feature: " << name << " = " << (enabled ? "true" : "false")
              << '\n';
  }

  std::cout << "sources:";
  for (const std::string& src : args.sources.value()) {
    std::cout << ' ' << src;
  }
  std::cout << '\n';

  return 0;
}
