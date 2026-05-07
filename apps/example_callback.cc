// example_callback.cc — demonstrates on_parse callbacks in ArgParameter.
//
// Each option/positional can carry a std::function<void(const value_type&)>
// that is called once per option *occurrence* (not once per value token).
// For positionals it is called once after all tokens are consumed.
// This is useful for side effects: logging, validation, or writing to an
// external variable via capture.
//
// Run:
//   ./example_callback --verbose --jobs 4 --include src lib --include inc
//   main.cc other.cc

#include <iostream>

#include "cli/argument.hh"
#include "cli/parser.hh"

struct Args {
  cli::Flag<"verbose", 'v'> verbose;
  cli::IntOption<"jobs", 'j'> jobs;
  cli::ListOption<std::string, "include", 'I'> includes;
  cli::Positional<std::string, cli::nargs::one_or_more> inputs;
};

auto main(int argc, char* argv[]) -> int {
  // External variables that callbacks will write into.
  bool verbose_flag = false;
  int job_count = 1;

  Args args;

  // Flag callback: fires with the stored bool when --verbose is seen.
  args.verbose = cli::Flag<"verbose", 'v'>{{
      .on_parse =
          [&verbose_flag](const bool& v) {
            verbose_flag = v;
            std::cerr << "[callback] verbose set to " << std::boolalpha << v
                      << '\n';
          },
  }};

  // Option callback: fires with std::optional<int> after --jobs N is parsed.
  args.jobs = cli::IntOption<"jobs", 'j'>{{
      .help = "parallel job count",
      .on_parse =
          [&job_count](const std::optional<int>& v) {
            if (v) {
              job_count = *v;
              std::cerr << "[callback] jobs set to " << *v << '\n';
            }
          },
  }};

  // List option callback: fires once per --include occurrence (after all its
  // values are consumed).  The callback receives the full accumulated vector.
  args.includes = cli::ListOption<std::string, "include", 'I'>{{
      .help = "include directories",
      .on_parse =
          [](const std::vector<std::string>& v) {
            std::cerr << "[callback] --include done, total " << v.size()
                      << " dir(s), last added: " << v.back() << '\n';
          },
  }};

  // Positional callback: fires once after all positional tokens are consumed.
  args.inputs = cli::Positional<std::string, cli::nargs::one_or_more>{{
      .help = "input files",
      .on_parse =
          [](const std::vector<std::string>& v) {
            std::cerr << "[callback] all inputs consumed (" << v.size()
                      << " file(s))\n";
          },
  }};

  // Parse — pass the pre-initialized args to preserve the callbacks.
  std::vector<std::string_view> argv_sv;
  for (int i = 0; i < argc; ++i) argv_sv.emplace_back(argv[i]);  // NOLINT

  auto result = cli::Parser<Args>{}.parse(
      std::move(args), std::span<const std::string_view>(argv_sv), 1);

  if (!result) {
    std::cerr << result.error.message() << '\n';
    return result.error.exit_code();
  }

  std::cout << "\n--- parsed results ---\n";
  std::cout << "verbose (via callback): " << std::boolalpha << verbose_flag
            << '\n';
  std::cout << "jobs    (via callback): " << job_count << '\n';
  std::cout << "includes (from result.value):\n";
  for (const auto& inc : result.value.includes.value()) {
    std::cout << "  " << inc << '\n';
  }
  std::cout << "inputs:\n";
  for (const auto& f : result.value.inputs.value()) {
    std::cout << "  " << f << '\n';
  }

  return 0;
}
