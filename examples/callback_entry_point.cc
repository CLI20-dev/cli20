// callback_entry_point.cc — demonstrates using on_parse callbacks to populate
// an application state object before handing off to the real entry point.

#include <iostream>

#include "cli/argument.hh"
#include "cli/parser.hh"

struct AppState {
  bool verbose = false;
  int jobs = 1;
  std::vector<std::string> includes;
  std::vector<std::string> inputs;
};

struct Args {
  cli::Description description{
      "Populate application state via callbacks, then call the real entry "
      "point."};
  cli::Help<> help;
  cli::Flag<"verbose", 'v'> verbose;
  cli::IntOption<"jobs", 'j'> jobs;
  cli::ListOption<std::string, "include", 'I', cli::nargs::exactly<1>> includes;
  cli::Positional<std::string, cli::nargs::one_or_more> inputs;
};

auto run(const AppState& state) -> int {
  std::cout << "verbose: " << std::boolalpha << state.verbose << '\n';
  std::cout << "jobs: " << state.jobs << '\n';
  for (const auto& include_dir : state.includes) {
    std::cout << "include: " << include_dir << '\n';
  }
  for (const auto& input : state.inputs) {
    std::cout << "input: " << input << '\n';
  }
  return 0;
}

auto main(int argc, char* argv[]) -> int {
  AppState state;
  Args args;

  args.verbose = cli::Flag<"verbose", 'v'>{{
      .on_parse = [&state](const bool& value) -> void { state.verbose = value; },
  }};
  args.jobs = cli::IntOption<"jobs", 'j'>{{
      .on_parse = [&state](const int& value) -> void { state.jobs = value; },
  }};
  args.includes =
      cli::ListOption<std::string, "include", 'I', cli::nargs::exactly<1>>{{
          .on_parse = [&state](const std::vector<std::string>& value) -> void {
            state.includes = value;
          },
      }};
  args.inputs = cli::Positional<std::string, cli::nargs::one_or_more>{{
      .on_parse = [&state](const std::vector<std::string>& value) -> void {
        state.inputs = value;
      },
  }};

  const auto result = cli::Parser<Args>{}.parse(std::move(args), argc, argv);
  if (!result) {
    std::cerr << result.error.message() << '\n';
    return result.error.exit_code();
  }

  return run(state);
}
