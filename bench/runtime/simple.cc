#include <benchmark/benchmark.h>

#include <CLI/CLI.hpp>
#include <argparse/argparse.hpp>
#include <cxxopts.hpp>
#include <string>

#include "bench/argv.hh"
#include "cli/argument.hh"
#include "cli/parser.hh"

namespace {

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
  for (auto _ : state) {
    auto result = cli::Parser<Cli20Simple>{}.parse(args.span(), 1);
    benchmark::DoNotOptimize(result);
  }
}

void BM_Cli11Simple(benchmark::State& state) {
  auto& args = simple_argv();
  for (auto _ : state) {
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
  }
}

void BM_ArgparseSimple(benchmark::State& state) {
  auto& args = simple_argv();
  for (auto _ : state) {
    argparse::ArgumentParser program{"simple"};
    program.add_argument("-v", "--verbose").flag();
    program.add_argument("-o", "--output");
    program.add_argument("input");
    program.parse_args(args.argc(), args.argv());
    benchmark::DoNotOptimize(program);
  }
}

void BM_CxxoptsSimple(benchmark::State& state) {
  auto& args = simple_argv();
  for (auto _ : state) {
    cxxopts::Options options{"simple"};
    options.add_options()("v,verbose", "verbose",
                          cxxopts::value<bool>()->default_value("false"))(
        "o,output", "output", cxxopts::value<std::string>())(
        "input", "input", cxxopts::value<std::string>());
    options.parse_positional({"input"});
    auto result = options.parse(args.argc(), args.argv());
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_Cli20Simple);
BENCHMARK(BM_Cli11Simple);
BENCHMARK(BM_ArgparseSimple);
BENCHMARK(BM_CxxoptsSimple);

}  // namespace
