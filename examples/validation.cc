#include <iostream>

#include "cli/argument.hh"
#include "cli/parser.hh"

struct Args {
  cli::Description description{
      "Validation-focused example covering range, regex, and filesystem "
      "checks."};
  cli::Help<> help;

  cli::Arg<"port", 'p',
           cli::conversion::integer<int> | cli::validation::range<1, 65535> |
               cli::pack::set_once>
      port{{.help = "TCP port number", .presence = cli::required}};

  cli::Arg<"profile", cli::conversion::string |
                          cli::validation::matches<"[a-z][a-z0-9_-]*"> |
                          cli::pack::set_once>
      profile{{.help = "Lowercase profile name", .presence = cli::required}};

  cli::Arg<"output", 'o',
           cli::conversion::path | cli::validation::parent_exists |
               cli::pack::set_once>
      output{{.help = "Output path whose parent must already exist"}};
};

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parse_or_exit<Args>(argc, argv);

  std::cout << "port: " << *args.port.value() << '\n';
  std::cout << "profile: " << *args.profile.value() << '\n';
  if (args.output.value()) {
    std::cout << "output: " << args.output.value()->string() << '\n';
  }
  return 0;
}
