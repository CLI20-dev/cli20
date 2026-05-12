add_executable(bench_runtime
  allocation_count.cc
  simple.cc
  many_options.cc
  subcommand.cc
)

target_link_libraries(bench_runtime PRIVATE
  bench_common
  bench_warnings
  bench_cli11
  bench_argparse
  bench_cxxopts
  bench_boost_program_options
  benchmark::benchmark
  benchmark::benchmark_main
)
