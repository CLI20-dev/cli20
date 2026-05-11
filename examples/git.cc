#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cli/argument.hh"
#include "cli/parser.hh"

namespace fs = std::filesystem;

constexpr auto is_color_when = [](const std::string& value) {
  return value == "always" || value == "auto" || value == "never";
};

struct CloneArgs {
  cli::Description description{"Clones repos."};
  cli::Help<> help;
  cli::Positional<std::string> remote{
      {.help = "The remote to clone", .presence = cli::required}};
};

struct DiffArgs {
  cli::Description description{"Compare two commits."};
  cli::Help<> help;

  cli::Arg<"color",
           cli::conversion::default_missing_value<"always"> |
               cli::conversion::string |
               cli::validation::predicate<is_color_when> | cli::pack::set_once,
           cli::nargs::zero_or_one>
      color{{.help = "When to use color: always, auto, or never",
             .default_value = std::string{"auto"}}};

  cli::Positional<std::string> base{{.help = "Base commit"}};
  cli::Positional<std::string> head{{.help = "Head commit"}};
  cli::Positional<std::string> path{{.help = "Path to diff"}};
};

struct PushArgs {
  cli::Description description{"Pushes things."};
  cli::Help<> help;
  cli::Positional<std::string> remote{
      {.help = "The remote to target", .presence = cli::required}};
};

struct AddArgs {
  cli::Description description{"Adds things."};
  cli::Help<> help;
  cli::Positional<fs::path, cli::nargs::one_or_more> paths{
      {.help = "Stuff to add", .presence = cli::required}};
};

struct StashPushArgs {
  cli::Description description{"Push a new stash."};
  cli::Help<> help;
  cli::StringOption<"message", 'm'> message{{.help = "Stash message"}};
};

struct StashPopArgs {
  cli::Description description{"Pop a stash."};
  cli::Help<> help;
  cli::Positional<std::string> stash{{.help = "Stash reference"}};
};

struct StashApplyArgs {
  cli::Description description{"Apply a stash."};
  cli::Help<> help;
  cli::Positional<std::string> stash{{.help = "Stash reference"}};
};

struct StashArgs {
  cli::Description description{"Manage the stash."};
  cli::Help<> help;
  cli::StringOption<"message", 'm'> message{{.help = "Stash message"}};
  cli::Command<"push", StashPushArgs> push{{.help = "Push a new stash"}};
  cli::Command<"pop", StashPopArgs> pop{{.help = "Pop a stash"}};
  cli::Command<"apply", StashApplyArgs> apply{{.help = "Apply a stash"}};
};

struct Args {
  cli::Description description{"A fictional versioning CLI."};
  cli::Help<> help;
  cli::Command<"clone", CloneArgs> clone{{.help = "Clones repos"}};
  cli::Command<"diff", DiffArgs> diff{{.help = "Compare two commits"}};
  cli::Command<"push", PushArgs> push{{.help = "Pushes things"}};
  cli::Command<"add", AddArgs> add{{.help = "Adds things"}};
  cli::Command<"stash", StashArgs> stash{{.help = "Manage the stash"}};
};

auto optional_string(const auto& arg) -> std::optional<std::string_view> {
  if (!arg) return std::nullopt;
  return std::string_view(*arg);
}

auto print_paths(const std::vector<fs::path>& paths) -> void {
  std::cout << "[";
  for (std::size_t i = 0; i < paths.size(); ++i) {
    if (i != 0) std::cout << ", ";
    std::cout << '"' << paths[i].string() << '"';
  }
  std::cout << "]";
}

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parse_or_exit<Args>(argc, argv);

  if (args.clone.provided()) {
    std::cout << "Cloning " << *args.clone.remote << '\n';
    return 0;
  }

  if (args.diff.provided()) {
    auto base = optional_string(args.diff.base);
    auto head = optional_string(args.diff.head);
    auto path = optional_string(args.diff.path);

    if (!path) {
      path = head;
      head = std::nullopt;
      if (!path) {
        path = base;
        base = std::nullopt;
      }
    }

    std::cout << "Diffing " << base.value_or("stage") << ".."
              << head.value_or("worktree") << " " << path.value_or("")
              << " (color=" << args.diff.color.value() << ")\n";
    return 0;
  }

  if (args.push.provided()) {
    std::cout << "Pushing to " << *args.push.remote << '\n';
    return 0;
  }

  if (args.add.provided()) {
    std::cout << "Adding ";
    print_paths(args.add.paths.value());
    std::cout << '\n';
    return 0;
  }

  if (args.stash.provided()) {
    if (args.stash.apply.provided()) {
      std::cout << "Applying ";
      if (args.stash.apply.stash) {
        std::cout << '"' << *args.stash.apply.stash << '"';
      } else {
        std::cout << "null";
      }
      std::cout << '\n';
      return 0;
    }

    if (args.stash.pop.provided()) {
      std::cout << "Popping ";
      if (args.stash.pop.stash) {
        std::cout << '"' << *args.stash.pop.stash << '"';
      } else {
        std::cout << "null";
      }
      std::cout << '\n';
      return 0;
    }

    const auto& message = args.stash.push.provided() ? args.stash.push.message
                                                     : args.stash.message;
    std::cout << "Pushing ";
    if (message) {
      std::cout << '"' << *message << '"';
    } else {
      std::cout << "null";
    }
    std::cout << '\n';
    return 0;
  }

  std::cerr << "No command provided. Try --help.\n";
  return 1;
}
