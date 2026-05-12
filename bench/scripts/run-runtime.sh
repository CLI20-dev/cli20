#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
cxx_standard="${BENCH_CXX_STANDARD:-20}"
build_dir="${BUILD_DIR:-$repo_root/build-bench/cxx$cxx_standard}"
results_dir="${RESULTS_DIR:-$repo_root/bench/results}"

cmake -S "$repo_root/bench" -B "$build_dir" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBENCH_CXX_STANDARD="$cxx_standard"
cmake --build "$build_dir" --target bench_runtime

mkdir -p "$results_dir"
"$build_dir/runtime/bench_runtime" \
  --benchmark_out="$results_dir/runtime_cxx$cxx_standard.json" \
  --benchmark_out_format=json \
  "$@"
