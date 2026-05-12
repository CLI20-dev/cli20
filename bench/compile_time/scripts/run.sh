#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/../../.." && pwd)"
out_dir="${1:-$repo_root/bench/results/compile_time}"

cases=(simple many_options subcommand)
libraries=(cli20 cli11 argparse cxxopts)

mkdir -p "$out_dir"
result_file="$out_dir/results.tsv"
printf "case\tlibrary\tseconds\n" > "$result_file"

for case_name in "${cases[@]}"; do
  for library in "${libraries[@]}"; do
    object_dir="$out_dir/objects"
    rm -f "$object_dir/${case_name}_${library}.o"
    start="$(python3 -c 'import time; print(time.perf_counter())')"
    "$repo_root/bench/compile_time/scripts/compile-one.sh" \
      "$case_name" "$library" "$object_dir"
    end="$(python3 -c 'import time; print(time.perf_counter())')"
    elapsed="$(python3 -c 'import sys; print(f"{float(sys.argv[2]) - float(sys.argv[1]):.6f}")' "$start" "$end")"
    printf "%s\t%s\t%s\n" "$case_name" "$library" "$elapsed" | tee -a "$result_file"
  done
done
