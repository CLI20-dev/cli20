#include <benchmark/benchmark.h>

#include <CLI/CLI.hpp>
#include <argparse/argparse.hpp>
#include <boost/program_options.hpp>
#include <cxxopts.hpp>
#include <string>

#include "bench/allocation_counter.hh"
#include "bench/argv.hh"
#include "cli/argument.hh"
#include "cli/parser.hh"

namespace {

namespace po = boost::program_options;

struct Cli20Build {
  cli::StringOption<"profile"> profile;
};

struct Cli20Test {
  cli::Flag<"verbose", 'v'> verbose;
};

struct Cli20Subcommand {
  cli::Command<"build", Cli20Build> build;
  cli::Command<"test", Cli20Test> test;
};

auto subcommand_argv() -> bench::Argv& {
  static bench::Argv args{"prog", "build", "--profile", "release"};
  return args;
}

void BM_Cli20Subcommand(benchmark::State& state) {
  auto& args = subcommand_argv();
  std::size_t allocations = 0;
  std::size_t bytes = 0;
  for (auto _ : state) {
    const auto stats = bench::measure_allocations([&] {
      auto result = cli::Parser<Cli20Subcommand>{}.parse(args.span(), 1);
      benchmark::DoNotOptimize(result);
    });
    allocations += stats.allocations;
    bytes += stats.bytes;
  }
  bench::set_allocation_counters(state, allocations, bytes);
}

void BM_Cli11Subcommand(benchmark::State& state) {
  auto& args = subcommand_argv();
  std::size_t allocations = 0;
  std::size_t bytes = 0;
  for (auto _ : state) {
    const auto stats = bench::measure_allocations([&] {
      CLI::App app{"subcommand"};
      std::string profile;
      bool verbose = false;
      auto* build = app.add_subcommand("build");
      build->add_option("--profile", profile);
      auto* test = app.add_subcommand("test");
      test->add_flag("-v,--verbose", verbose);
      app.parse(args.argc(), args.argv());
      benchmark::DoNotOptimize(profile);
      benchmark::DoNotOptimize(verbose);
    });
    allocations += stats.allocations;
    bytes += stats.bytes;
  }
  bench::set_allocation_counters(state, allocations, bytes);
}

void BM_ArgparseSubcommand(benchmark::State& state) {
  auto& args = subcommand_argv();
  std::size_t allocations = 0;
  std::size_t bytes = 0;
  for (auto _ : state) {
    const auto stats = bench::measure_allocations([&] {
      argparse::ArgumentParser program{"subcommand"};
      argparse::ArgumentParser build{"build"};
      build.add_argument("--profile");
      argparse::ArgumentParser test{"test"};
      test.add_argument("-v", "--verbose").flag();
      program.add_subparser(build);
      program.add_subparser(test);
      program.parse_args(args.argc(), args.argv());
      benchmark::DoNotOptimize(program);
    });
    allocations += stats.allocations;
    bytes += stats.bytes;
  }
  bench::set_allocation_counters(state, allocations, bytes);
}

void BM_CxxoptsSubcommand(benchmark::State& state) {
  auto& args = subcommand_argv();
  std::size_t allocations = 0;
  std::size_t bytes = 0;
  for (auto _ : state) {
    const auto stats = bench::measure_allocations([&] {
      const std::string command = args.storage[1];
      if (command == "build") {
        cxxopts::Options options{"build"};
        options.add_options()("profile", "profile",
                              cxxopts::value<std::string>());
        auto result = options.parse(args.argc() - 1, args.argv() + 1);
        benchmark::DoNotOptimize(result);
      }
    });
    allocations += stats.allocations;
    bytes += stats.bytes;
  }
  bench::set_allocation_counters(state, allocations, bytes);
}

void BM_BoostSubcommand(benchmark::State& state) {
  auto& args = subcommand_argv();
  std::size_t allocations = 0;
  std::size_t bytes = 0;
  for (auto _ : state) {
    const auto stats = bench::measure_allocations([&] {
      const std::string command = args.storage[1];
      if (command == "build") {
        std::string profile;
        po::options_description options{"build"};
        options.add_options()("profile", po::value<std::string>(&profile));
        po::variables_map values;
        po::store(po::command_line_parser(args.argc() - 1, args.argv() + 1)
                      .options(options)
                      .run(),
                  values);
        po::notify(values);
        benchmark::DoNotOptimize(profile);
      }
    });
    allocations += stats.allocations;
    bytes += stats.bytes;
  }
  bench::set_allocation_counters(state, allocations, bytes);
}

BENCHMARK(BM_Cli20Subcommand);
BENCHMARK(BM_Cli11Subcommand);
BENCHMARK(BM_ArgparseSubcommand);
BENCHMARK(BM_CxxoptsSubcommand);
BENCHMARK(BM_BoostSubcommand);

}  // namespace
