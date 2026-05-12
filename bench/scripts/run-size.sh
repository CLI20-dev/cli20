#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
build_dir="${BUILD_DIR:-$repo_root/build-bench}"
results_dir="$repo_root/bench/results"

cmake -S "$repo_root/bench" -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" --target all

mkdir -p "$results_dir"
result_file="$results_dir/size.tsv"
printf "case\tlibrary\tbytes\n" > "$result_file"

for binary in "$build_dir"/size/bench_size_*; do
  [ -x "$binary" ] || continue
  name="$(basename "$binary")"
  rest="${name#bench_size_}"
  library="${rest##*_}"
  case_name="${rest%_$library}"
  bytes="$(wc -c < "$binary" | tr -d ' ')"
  printf "%s\t%s\t%s\n" "$case_name" "$library" "$bytes" | tee -a "$result_file"
done
