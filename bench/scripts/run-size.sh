#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
cxx_standard="${BENCH_CXX_STANDARD:-20}"
build_dir="${BUILD_DIR:-$repo_root/build-bench/cxx$cxx_standard}"
results_dir="${RESULTS_DIR:-$repo_root/bench/results}"
strip_cmd="${STRIP:-strip}"

cmake -S "$repo_root/bench" -B "$build_dir" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBENCH_CXX_STANDARD="$cxx_standard"
cmake --build "$build_dir" --target all

mkdir -p "$results_dir"
result_file="$results_dir/size_cxx$cxx_standard.tsv"
stripped_dir="$results_dir/stripped/cxx$cxx_standard"
mkdir -p "$stripped_dir"
printf "standard\tcase\tlibrary\tunstripped_bytes\tstripped_bytes\n" > "$result_file"

for binary in "$build_dir"/size/bench_size_*; do
  [ -x "$binary" ] || continue
  name="$(basename "$binary")"
  rest="${name#bench_size_}"
  library="${rest##*_}"
  case_name="${rest%_$library}"
  bytes="$(wc -c < "$binary" | tr -d ' ')"
  stripped="$stripped_dir/$name"
  cp "$binary" "$stripped"
  if "$strip_cmd" "$stripped" >/dev/null 2>&1; then
    stripped_bytes="$(wc -c < "$stripped" | tr -d ' ')"
  else
    stripped_bytes="strip_failed"
  fi
  printf "c++%s\t%s\t%s\t%s\t%s\n" \
    "$cxx_standard" "$case_name" "$library" "$bytes" "$stripped_bytes" |
    tee -a "$result_file"
done
