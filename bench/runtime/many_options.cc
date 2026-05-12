#include <benchmark/benchmark.h>

#include <CLI/CLI.hpp>
#include <argparse/argparse.hpp>
#include <cxxopts.hpp>
#include <string>

#include "bench/argv.hh"
#include "cli/argument.hh"
#include "cli/parser.hh"

namespace {

struct Cli20ManyOptions {
  cli::StringOption<"opt01"> opt01;
  cli::StringOption<"opt02"> opt02;
  cli::StringOption<"opt03"> opt03;
  cli::StringOption<"opt04"> opt04;
  cli::StringOption<"opt05"> opt05;
  cli::StringOption<"opt06"> opt06;
  cli::StringOption<"opt07"> opt07;
  cli::StringOption<"opt08"> opt08;
  cli::StringOption<"opt09"> opt09;
  cli::StringOption<"opt10"> opt10;
  cli::StringOption<"opt11"> opt11;
  cli::StringOption<"opt12"> opt12;
  cli::StringOption<"opt13"> opt13;
  cli::StringOption<"opt14"> opt14;
  cli::StringOption<"opt15"> opt15;
  cli::StringOption<"opt16"> opt16;
};

auto many_argv() -> bench::Argv& {
  static bench::Argv args{"prog", "--opt01", "v01", "--opt02", "v02", "--opt03",
                          "v03",  "--opt04", "v04", "--opt05", "v05", "--opt06",
                          "v06",  "--opt07", "v07", "--opt08", "v08", "--opt09",
                          "v09",  "--opt10", "v10", "--opt11", "v11", "--opt12",
                          "v12",  "--opt13", "v13", "--opt14", "v14", "--opt15",
                          "v15",  "--opt16", "v16"};
  return args;
}

void add_cli11_options(CLI::App& app, std::string (&values)[16]) {
  app.add_option("--opt01", values[0]);
  app.add_option("--opt02", values[1]);
  app.add_option("--opt03", values[2]);
  app.add_option("--opt04", values[3]);
  app.add_option("--opt05", values[4]);
  app.add_option("--opt06", values[5]);
  app.add_option("--opt07", values[6]);
  app.add_option("--opt08", values[7]);
  app.add_option("--opt09", values[8]);
  app.add_option("--opt10", values[9]);
  app.add_option("--opt11", values[10]);
  app.add_option("--opt12", values[11]);
  app.add_option("--opt13", values[12]);
  app.add_option("--opt14", values[13]);
  app.add_option("--opt15", values[14]);
  app.add_option("--opt16", values[15]);
}

void add_argparse_options(argparse::ArgumentParser& program) {
  program.add_argument("--opt01");
  program.add_argument("--opt02");
  program.add_argument("--opt03");
  program.add_argument("--opt04");
  program.add_argument("--opt05");
  program.add_argument("--opt06");
  program.add_argument("--opt07");
  program.add_argument("--opt08");
  program.add_argument("--opt09");
  program.add_argument("--opt10");
  program.add_argument("--opt11");
  program.add_argument("--opt12");
  program.add_argument("--opt13");
  program.add_argument("--opt14");
  program.add_argument("--opt15");
  program.add_argument("--opt16");
}

void add_cxxopts_options(cxxopts::Options& options) {
  options.add_options()("opt01", "opt01", cxxopts::value<std::string>())(
      "opt02", "opt02", cxxopts::value<std::string>())(
      "opt03", "opt03", cxxopts::value<std::string>())(
      "opt04", "opt04", cxxopts::value<std::string>())(
      "opt05", "opt05", cxxopts::value<std::string>())(
      "opt06", "opt06", cxxopts::value<std::string>())(
      "opt07", "opt07", cxxopts::value<std::string>())(
      "opt08", "opt08", cxxopts::value<std::string>())(
      "opt09", "opt09", cxxopts::value<std::string>())(
      "opt10", "opt10", cxxopts::value<std::string>())(
      "opt11", "opt11", cxxopts::value<std::string>())(
      "opt12", "opt12", cxxopts::value<std::string>())(
      "opt13", "opt13", cxxopts::value<std::string>())(
      "opt14", "opt14", cxxopts::value<std::string>())(
      "opt15", "opt15", cxxopts::value<std::string>())(
      "opt16", "opt16", cxxopts::value<std::string>());
}

void BM_Cli20ManyOptions(benchmark::State& state) {
  auto& args = many_argv();
  for (auto _ : state) {
    auto result = cli::Parser<Cli20ManyOptions>{}.parse(args.span(), 1);
    benchmark::DoNotOptimize(result);
  }
}

void BM_Cli11ManyOptions(benchmark::State& state) {
  auto& args = many_argv();
  for (auto _ : state) {
    CLI::App app{"many_options"};
    std::string values[16];
    add_cli11_options(app, values);
    app.parse(args.argc(), args.argv());
    benchmark::DoNotOptimize(values);
  }
}

void BM_ArgparseManyOptions(benchmark::State& state) {
  auto& args = many_argv();
  for (auto _ : state) {
    argparse::ArgumentParser program{"many_options"};
    add_argparse_options(program);
    program.parse_args(args.argc(), args.argv());
    benchmark::DoNotOptimize(program);
  }
}

void BM_CxxoptsManyOptions(benchmark::State& state) {
  auto& args = many_argv();
  for (auto _ : state) {
    cxxopts::Options options{"many_options"};
    add_cxxopts_options(options);
    auto result = options.parse(args.argc(), args.argv());
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_Cli20ManyOptions);
BENCHMARK(BM_Cli11ManyOptions);
BENCHMARK(BM_ArgparseManyOptions);
BENCHMARK(BM_CxxoptsManyOptions);

}  // namespace
