#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>

#include "cli/argument.hh"
#include "cli/parser.hh"

namespace fs = std::filesystem;

auto nproc_dummy() -> int {
  // Example-only stand-in for a runtime nproc query. Kept deterministic so it
  // works in environments where querying host CPU count is awkward.
  return 4;
}

constexpr auto jobs_or_nproc =
    [](cli::ActionCtx<void> ctx,
       cli::ActionResult<std::optional<std::string_view>> input)
    -> cli::ActionResult<int> {
  if (!input.have_value()) return cli::fail<int>(input.error);
  if (input.value.has_value()) {
    return cli::conversion::integer<int>.invoke(
        ctx, cli::ActionResult<std::string_view>::ok(input.value.value()));
  }
  return cli::ActionResult<int>::ok(nproc_dummy());
};

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
           cli::action::custom<jobs_or_nproc> | cli::validation::range<1, 64> |
               cli::pack::set_once,
           cli::nargs::zero_or_one>
      jobs_arg{{.help = "Parallel job count (default 1; -j uses nproc_dummy())",
                .default_value = 1}};

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

  std::cout << "config: " << args.config->string() << '\n';
  std::cout << "jobs: " << *args.jobs_arg << '\n';
  for (const fs::path& include_dir : args.includes.value()) {
    std::cout << "include: " << include_dir.string() << '\n';
  }
  for (const std::string& define : args.defines.value()) {
    std::cout << "define: " << define << '\n';
  }
  return 0;
}
