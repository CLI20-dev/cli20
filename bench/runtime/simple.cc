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

struct Cli20Simple {
  cli::Flag<"verbose", 'v'> verbose;
  cli::StringOption<"output", 'o'> output;
  cli::Positional<std::string> input{{.presence = cli::required}};
};

auto simple_argv() -> bench::Argv& {
  static bench::Argv args{"prog", "-v", "-o", "out.txt", "input.txt"};
  return args;
}

void BM_Cli20Simple(benchmark::State& state) {
  auto& args = simple_argv();
  std::size_t allocations = 0;
  std::size_t bytes = 0;
  for (auto _ : state) {
    const auto stats = bench::measure_allocations([&] {
      auto result = cli::Parser<Cli20Simple>{}.parse(args.span(), 1);
      benchmark::DoNotOptimize(result);
    });
    allocations += stats.allocations;
    bytes += stats.bytes;
  }
  bench::set_allocation_counters(state, allocations, bytes);
}

void BM_Cli11Simple(benchmark::State& state) {
  auto& args = simple_argv();
  std::size_t allocations = 0;
  std::size_t bytes = 0;
  for (auto _ : state) {
    const auto stats = bench::measure_allocations([&] {
      CLI::App app{"simple"};
      bool verbose = false;
      std::string output;
      std::string input;
      app.add_flag("-v,--verbose", verbose);
      app.add_option("-o,--output", output);
      app.add_option("input", input)->required();
      app.parse(args.argc(), args.argv());
      benchmark::DoNotOptimize(verbose);
      benchmark::DoNotOptimize(output);
      benchmark::DoNotOptimize(input);
    });
    allocations += stats.allocations;
    bytes += stats.bytes;
  }
  bench::set_allocation_counters(state, allocations, bytes);
}

void BM_ArgparseSimple(benchmark::State& state) {
  auto& args = simple_argv();
  std::size_t allocations = 0;
  std::size_t bytes = 0;
  for (auto _ : state) {
    const auto stats = bench::measure_allocations([&] {
      argparse::ArgumentParser program{"simple"};
      program.add_argument("-v", "--verbose").flag();
      program.add_argument("-o", "--output");
      program.add_argument("input");
      program.parse_args(args.argc(), args.argv());
      benchmark::DoNotOptimize(program);
    });
    allocations += stats.allocations;
    bytes += stats.bytes;
  }
  bench::set_allocation_counters(state, allocations, bytes);
}

void BM_CxxoptsSimple(benchmark::State& state) {
  auto& args = simple_argv();
  std::size_t allocations = 0;
  std::size_t bytes = 0;
  for (auto _ : state) {
    const auto stats = bench::measure_allocations([&] {
      cxxopts::Options options{"simple"};
      options.add_options()("v,verbose", "verbose",
                            cxxopts::value<bool>()->default_value("false"))(
          "o,output", "output", cxxopts::value<std::string>())(
          "input", "input", cxxopts::value<std::string>());
      options.parse_positional({"input"});
      auto result = options.parse(args.argc(), args.argv());
      benchmark::DoNotOptimize(result);
    });
    allocations += stats.allocations;
    bytes += stats.bytes;
  }
  bench::set_allocation_counters(state, allocations, bytes);
}

void BM_BoostSimple(benchmark::State& state) {
  auto& args = simple_argv();
  std::size_t allocations = 0;
  std::size_t bytes = 0;
  for (auto _ : state) {
    const auto stats = bench::measure_allocations([&] {
      bool verbose = false;
      std::string output;
      std::string input;
      po::options_description options{"simple"};
      options.add_options()("verbose,v", po::bool_switch(&verbose))(
          "output,o", po::value<std::string>(&output))(
          "input", po::value<std::string>(&input)->required());
      po::positional_options_description positional;
      positional.add("input", 1);
      po::variables_map values;
      po::store(po::command_line_parser(args.argc(), args.argv())
                    .options(options)
                    .positional(positional)
                    .run(),
                values);
      po::notify(values);
      benchmark::DoNotOptimize(verbose);
      benchmark::DoNotOptimize(output);
      benchmark::DoNotOptimize(input);
    });
    allocations += stats.allocations;
    bytes += stats.bytes;
  }
  bench::set_allocation_counters(state, allocations, bytes);
}

BENCHMARK(BM_Cli20Simple);
BENCHMARK(BM_Cli11Simple);
BENCHMARK(BM_ArgparseSimple);
BENCHMARK(BM_CxxoptsSimple);
BENCHMARK(BM_BoostSimple);

}  // namespace
