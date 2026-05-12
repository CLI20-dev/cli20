#include <iostream>

#include "cli/argument.hh"
#include "cli/parser.hh"

struct Args {
  cli::Description description{
      "Relation metadata example covering groups, conflicts, and depends_on."};
  cli::Help<> help;

  cli::StringOption<"user"> user{{.help = "Legacy auth user name"}};
  cli::StringOption<"password"> password{{.help = "Legacy auth password"}};

  cli::Flag<"json"> json{{.help = "Write JSON output"}};
  cli::Flag<"markdown"> markdown{{.help = "Write Markdown output"}};

  cli::StringOption<"profile"> profile{{.help = "Deployment profile"}};

  static constexpr auto relations = cli::relations(
      cli::group({.name = "legacy_auth", .members = {"user", "password"}}),
      cli::conflicts("json", "markdown"),
      cli::depends_on("legacy_auth", "profile"));
};

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parse_or_exit<Args>(argc, argv);

  std::cout << "format: " << (args.json ? "json" : "markdown") << '\n';
  if (args.user) {
    std::cout << "legacy auth: " << *args.user << '\n';
  }
  if (args.profile) {
    std::cout << "profile: " << *args.profile << '\n';
  }
  return 0;
}
