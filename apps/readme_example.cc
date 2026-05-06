#include <argon/argument.hh>
#include <iostream>

#include "argon/parser.hh"

struct Argument {
  argon::Description description{
      "This is an example of using argon to parse command-line arguments."};

  argon::PositionalImpl<{.min = 1, .max = 1},
                        argon::Action<>{} |
                            argon::Action<argon::conversion::Integer<int>{}>{} |
                            argon::Action<argon::validation::Range<0, 100>{}>{} |
                            argon::Action<argon::pack::Push{}>{}>
      input_files{{.help = "The input files to process",
                   .presence = argon::Presence::required}};

  argon::ArgImpl<"help", 'h', {.min = 0, .max = 0},
                 argon::Action<>{} | argon::Action<argon::pack::Ignore{}>{}>
      help{{.help = "Show this help message and exit.",
            .presence = argon::Presence::optional,
            .default_value = {}}};

  // struct Build {
  //   argon::ArgImpl<"config", 'c', {.min = 0, .max = 0},
  //                  argon::action<int> | argon::always_true |
  //                      argon::always_string>
  //       config{{
  //           .help = "The configuration file for the build",
  //           .presence = argon::Presence::required,
  //       }};
  //
  //   argon::ArgImpl<"config2", 'd', {.min = 0, .max = 0},
  //                  argon::action<int> | argon::always_true |
  //                      argon::always_string>
  //       confi{{
  //           .help = "The configuration file for the build",
  //           .presence = argon::Presence::required,
  //       }};
  // };
  //
  // argon::Command<"build", Build> build{{
  //     .help = "Build the project with the specified configuration",
  // }};
};

template <argon::ArgumentSpec T>
struct test {};

// constexpr auto a = argon::Action<>{} |
//                    argon::Action<argon::conversion::Integer<int>{}>{} |
//                    argon::Action<argon::pack::Count{}>{};

auto main(int argc, char* argv[]) -> int {
  auto p = argon::Parser<Argument>{}.parse(argc, argv);

  // size_t x{};
  // auto ctx = argon::ActionCtx<size_t>{.arg = std::ref(x)};
  // auto res = argon::ActionResult<std::string_view>{.value = "42"};
  // std::ignore = a.invoke(ctx, res);
  // std::ignore = a.invoke(ctx, res);
  // std::ignore = a.invoke(ctx, res);
  // std::ignore = a.invoke(ctx, res);
  // std::cout << decltype(a)::validate().first << "\n";
  // std::print("x = {}\n", x);
  // for (int i : x) {
  //   std::cout << i << "\n";
  // }
  return 0;
}
