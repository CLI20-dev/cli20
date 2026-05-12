#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/../../.." && pwd)"
cxx_standard="${BENCH_CXX_STANDARD:-20}"
out_dir="${1:-$repo_root/bench/results/compile_time/cxx$cxx_standard}"
hyperfine_runs="${HYPERFINE_RUNS:-5}"
hyperfine_warmup="${HYPERFINE_WARMUP:-1}"

cases=(simple many_options subcommand)
libraries=(cli20 cli11 argparse cxxopts boost)

mkdir -p "$out_dir"
result_file="$out_dir/results.tsv"
json_dir="$out_dir/json"
object_dir="$out_dir/objects"
printf "standard\tcase\tlibrary\tseconds\n" > "$result_file"

for case_name in "${cases[@]}"; do
  for library in "${libraries[@]}"; do
    mkdir -p "$json_dir" "$object_dir"
    object_file="$object_dir/${case_name}_${library}.o"
    json_file="$json_dir/${case_name}_${library}.json"
    hyperfine \
      --warmup "$hyperfine_warmup" \
      --runs "$hyperfine_runs" \
      --prepare "rm -f '$object_file'" \
      --export-json "$json_file" \
      "bash $repo_root/bench/compile_time/scripts/compile-one.sh '$case_name' '$library' '$object_dir'" \
      >/dev/null
    elapsed="$(python3 -c 'import json, sys; print("{:.6f}".format(json.load(open(sys.argv[1]))["results"][0]["mean"]))' "$json_file")"
    printf "c++%s\t%s\t%s\t%s\n" "$cxx_standard" "$case_name" "$library" "$elapsed" |
      tee -a "$result_file"
  done
done
