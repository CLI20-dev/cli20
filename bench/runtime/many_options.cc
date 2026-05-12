#include <benchmark/benchmark.h>

#include <CLI/CLI.hpp>
#include <argparse/argparse.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/errors.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <cli/argument.hh>
#include <cli/parser.hh>
#include <cxxopts.hpp>
#include <filesystem>
#include <istream>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include "bench/allocation_counter.hh"
#include "bench/argv.hh"

namespace bench {

namespace po = boost::program_options;

enum class Mode { fast, balanced, precise };

inline auto mode_name(Mode mode) -> std::string_view {
  switch (mode) {
    case Mode::fast:
      return "fast";
    case Mode::balanced:
      return "balanced";
    case Mode::precise:
      return "precise";
  }
  return "unknown";
}

inline auto parse_mode(std::string_view value) -> std::optional<Mode> {
  if (value == "fast") return Mode::fast;
  if (value == "balanced") return Mode::balanced;
  if (value == "precise") return Mode::precise;
  return std::nullopt;
}

inline auto parse_mode_or_throw(const std::string& value) -> Mode {
  if (auto mode = parse_mode(value)) return *mode;
  throw std::runtime_error("invalid mode: " + value);
}

inline void validate(boost::any& out, const std::vector<std::string>& values,
                     Mode*, int) {
  po::validators::check_first_occurrence(out);
  const auto& value = po::validators::get_single_string(values);
  if (auto mode = parse_mode(value)) {
    out = *mode;
    return;
  }
  throw po::validation_error(po::validation_error::invalid_option_value, value);
}

inline auto operator>>(std::istream& in, Mode& mode) -> std::istream& {
  std::string value;
  in >> value;
  if (auto parsed = parse_mode(value)) {
    mode = *parsed;
  } else {
    in.setstate(std::ios::failbit);
  }
  return in;
}

inline auto operator<<(std::ostream& out, Mode mode) -> std::ostream& {
  return out << mode_name(mode);
}

struct ManyValues {
  std::string name;
  std::string output;
  std::string target;
  std::string define;
  int jobs{};
  int retries{};
  int port{};
  int depth{};
  double ratio{};
  double timeout{};
  double threshold{};
  double scale{};
  std::filesystem::path config;
  std::filesystem::path cache;
  Mode mode{};
  Mode color{};
};

}  // namespace bench

namespace {

struct Cli20ManyOptions {
  cli::StringOption<"name"> name;
  cli::StringOption<"output"> output;
  cli::StringOption<"target"> target;
  cli::StringOption<"define"> define;
  cli::IntOption<"jobs"> jobs;
  cli::IntOption<"retries"> retries;
  cli::IntOption<"port"> port;
  cli::IntOption<"depth"> depth;
  cli::DoubleOption<"ratio"> ratio;
  cli::DoubleOption<"timeout"> timeout;
  cli::DoubleOption<"threshold"> threshold;
  cli::DoubleOption<"scale"> scale;
  cli::PathOption<"config"> config;
  cli::PathOption<"cache"> cache;
  cli::Arg<"mode", cli::conversion::choice<bench::Mode, bench::parse_mode> |
                       cli::pack::set_once>
      mode;
  cli::Arg<"color", cli::conversion::choice<bench::Mode, bench::parse_mode> |
                        cli::pack::set_once>
      color;
};

auto many_argv() -> bench::Argv& {
  static bench::Argv args{
      "prog",     "--name",      "cli20",    "--output",    "out.o",
      "--target", "native",      "--define", "NDEBUG",      "--jobs",
      "12",       "--retries",   "3",        "--port",      "8080",
      "--depth",  "7",           "--ratio",  "0.75",        "--timeout",
      "2.5",      "--threshold", "0.001",    "--scale",     "1.25",
      "--config", "config.toml", "--cache",  "build/cache", "--mode",
      "balanced", "--color",     "fast"};
  return args;
}

void add_cli11_options(CLI::App& app, bench::ManyValues& values) {
  static const std::map<std::string, bench::Mode> modes{
      {"fast", bench::Mode::fast},
      {"balanced", bench::Mode::balanced},
      {"precise", bench::Mode::precise},
  };
  app.add_option("--name", values.name);
  app.add_option("--output", values.output);
  app.add_option("--target", values.target);
  app.add_option("--define", values.define);
  app.add_option("--jobs", values.jobs);
  app.add_option("--retries", values.retries);
  app.add_option("--port", values.port);
  app.add_option("--depth", values.depth);
  app.add_option("--ratio", values.ratio);
  app.add_option("--timeout", values.timeout);
  app.add_option("--threshold", values.threshold);
  app.add_option("--scale", values.scale);
  app.add_option("--config", values.config);
  app.add_option("--cache", values.cache);
  app.add_option("--mode", values.mode)
      ->transform(CLI::CheckedTransformer(modes));
  app.add_option("--color", values.color)
      ->transform(CLI::CheckedTransformer(modes));
}

void add_argparse_options(argparse::ArgumentParser& program) {
  program.add_argument("--name");
  program.add_argument("--output");
  program.add_argument("--target");
  program.add_argument("--define");
  program.add_argument("--jobs").scan<'i', int>();
  program.add_argument("--retries").scan<'i', int>();
  program.add_argument("--port").scan<'i', int>();
  program.add_argument("--depth").scan<'i', int>();
  program.add_argument("--ratio").scan<'g', double>();
  program.add_argument("--timeout").scan<'g', double>();
  program.add_argument("--threshold").scan<'g', double>();
  program.add_argument("--scale").scan<'g', double>();
  program.add_argument("--config").action([](const std::string& value) {
    return std::filesystem::path{value};
  });
  program.add_argument("--cache").action(
      [](const std::string& value) { return std::filesystem::path{value}; });
  program.add_argument("--mode").action(bench::parse_mode_or_throw);
  program.add_argument("--color").action(bench::parse_mode_or_throw);
}

void add_cxxopts_options(cxxopts::Options& options) {
  options.add_options()("name", "name", cxxopts::value<std::string>())(
      "output", "output", cxxopts::value<std::string>())(
      "target", "target", cxxopts::value<std::string>())(
      "define", "define", cxxopts::value<std::string>())("jobs", "jobs",
                                                         cxxopts::value<int>())(
      "retries", "retries", cxxopts::value<int>())("port", "port",
                                                   cxxopts::value<int>())(
      "depth", "depth", cxxopts::value<int>())("ratio", "ratio",
                                               cxxopts::value<double>())(
      "timeout", "timeout", cxxopts::value<double>())("threshold", "threshold",
                                                      cxxopts::value<double>())(
      "scale", "scale", cxxopts::value<double>())(
      "config", "config", cxxopts::value<std::filesystem::path>())(
      "cache", "cache", cxxopts::value<std::filesystem::path>())(
      "mode", "mode", cxxopts::value<bench::Mode>())(
      "color", "color", cxxopts::value<bench::Mode>());
}

void add_boost_options(boost::program_options::options_description& options,
                       bench::ManyValues& values) {
  namespace po = boost::program_options;
  options.add_options()("name", po::value<std::string>(&values.name))(
      "output", po::value<std::string>(&values.output))(
      "target", po::value<std::string>(&values.target))(
      "define", po::value<std::string>(&values.define))(
      "jobs", po::value<int>(&values.jobs))(
      "retries", po::value<int>(&values.retries))("port",
                                                  po::value<int>(&values.port))(
      "depth", po::value<int>(&values.depth))("ratio",
                                              po::value<double>(&values.ratio))(
      "timeout", po::value<double>(&values.timeout))(
      "threshold", po::value<double>(&values.threshold))(
      "scale", po::value<double>(&values.scale))(
      "config", po::value<std::filesystem::path>(&values.config))(
      "cache", po::value<std::filesystem::path>(&values.cache))(
      "mode", po::value<bench::Mode>(&values.mode))(
      "color", po::value<bench::Mode>(&values.color));
}

void BM_Cli20ManyOptions(benchmark::State& state) {
  auto& args = many_argv();
  std::size_t allocations = 0;
  std::size_t bytes = 0;
  for (auto _ : state) {
    const auto stats = bench::measure_allocations([&] {
      auto result = cli::Parser<Cli20ManyOptions>{}.parse(args.span(), 1);
      benchmark::DoNotOptimize(result);
    });
    allocations += stats.allocations;
    bytes += stats.bytes;
  }
  bench::set_allocation_counters(state, allocations, bytes);
}

void BM_Cli11ManyOptions(benchmark::State& state) {
  auto& args = many_argv();
  std::size_t allocations = 0;
  std::size_t bytes = 0;
  for (auto _ : state) {
    const auto stats = bench::measure_allocations([&] {
      CLI::App app{"many_options"};
      bench::ManyValues values;
      add_cli11_options(app, values);
      app.parse(args.argc(), args.argv());
      benchmark::DoNotOptimize(values);
    });
    allocations += stats.allocations;
    bytes += stats.bytes;
  }
  bench::set_allocation_counters(state, allocations, bytes);
}

void BM_ArgparseManyOptions(benchmark::State& state) {
  auto& args = many_argv();
  std::size_t allocations = 0;
  std::size_t bytes = 0;
  for (auto _ : state) {
    const auto stats = bench::measure_allocations([&] {
      argparse::ArgumentParser program{"many_options"};
      add_argparse_options(program);
      program.parse_args(args.argc(), args.argv());
      benchmark::DoNotOptimize(program);
    });
    allocations += stats.allocations;
    bytes += stats.bytes;
  }
  bench::set_allocation_counters(state, allocations, bytes);
}

void BM_CxxoptsManyOptions(benchmark::State& state) {
  auto& args = many_argv();
  std::size_t allocations = 0;
  std::size_t bytes = 0;
  for (auto _ : state) {
    const auto stats = bench::measure_allocations([&] {
      cxxopts::Options options{"many_options"};
      add_cxxopts_options(options);
      auto result = options.parse(args.argc(), args.argv());
      benchmark::DoNotOptimize(result);
    });
    allocations += stats.allocations;
    bytes += stats.bytes;
  }
  bench::set_allocation_counters(state, allocations, bytes);
}

void BM_BoostManyOptions(benchmark::State& state) {
  auto& args = many_argv();
  std::size_t allocations = 0;
  std::size_t bytes = 0;
  for (auto _ : state) {
    const auto stats = bench::measure_allocations([&] {
      boost::program_options::options_description options{"many_options"};
      bench::ManyValues values;
      add_boost_options(options, values);
      boost::program_options::variables_map parsed;
      boost::program_options::store(
          boost::program_options::command_line_parser(args.argc(), args.argv())
              .options(options)
              .run(),
          parsed);
      boost::program_options::notify(parsed);
      benchmark::DoNotOptimize(values);
    });
    allocations += stats.allocations;
    bytes += stats.bytes;
  }
  bench::set_allocation_counters(state, allocations, bytes);
}

BENCHMARK(BM_Cli20ManyOptions);
BENCHMARK(BM_Cli11ManyOptions);
BENCHMARK(BM_ArgparseManyOptions);
BENCHMARK(BM_CxxoptsManyOptions);
BENCHMARK(BM_BoostManyOptions);

}  // namespace
