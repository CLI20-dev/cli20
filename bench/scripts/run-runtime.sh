#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
build_dir="${BUILD_DIR:-$repo_root/build-bench}"
results_dir="$repo_root/bench/results"

cmake -S "$repo_root/bench" -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" --target bench_runtime

mkdir -p "$results_dir"
"$build_dir/runtime/bench_runtime" \
  --benchmark_out="$results_dir/runtime.json" \
  --benchmark_out_format=json \
  "$@"
