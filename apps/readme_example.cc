#include <argon/argument.hh>

struct Argument {
  argon::Description description{
      "This is an example of using argon to parse command-line arguments."};

  argon::PositionalImpl<{.min = 1, .max = 1},
                        argon::action | argon::always_true | argon::always_string>
      input_files{{.help = "The input files to process", .presence = argon::Presence::required}};

  argon::PositionalImpl<{.min = 1, .max = -1},
                        argon::action | argon::always_true | argon::always_string>
      input_files2{{.help = "The input files to process", .presence = argon::Presence::required}};

  argon::ArgImpl<"help", 'h', {.min = 0, .max = 0},
                 argon::action | argon::always_true | argon::always_string>
      help{{.help = "Show this help message and exit.",
            .presence = argon::Presence::optional,
            .default_value = false}};

  struct Build {
    argon::ArgImpl<"config", 'c', {.min = 0, .max = 0},
                   argon::action | argon::always_true | argon::always_string>
        config{{
            .help = "The configuration file for the build",
            .presence = argon::Presence::required,
        }};

    argon::ArgImpl<"config2", 'd', {.min = 0, .max = 0},
                   argon::action | argon::always_true | argon::always_string>
        confi{{
            .help = "The configuration file for the build",
            .presence = argon::Presence::required,
        }};
  };

  argon::Command<"build", Build> build{{
      .help = "Build the project with the specified configuration",
  }};
};

template <argon::ArgumentSpec T>
struct test {};

auto main() -> int {
  std::ignore = test<Argument>{};
  return 0;
}
