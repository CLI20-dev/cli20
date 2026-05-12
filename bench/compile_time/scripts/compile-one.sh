#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 3 ]; then
  echo "usage: $0 CASE LIBRARY OUTPUT_DIR" >&2
  exit 2
fi

case_name="$1"
library="$2"
output_dir="$3"

repo_root="$(cd "$(dirname "$0")/../../.." && pwd)"
source_file="$repo_root/bench/compile_time/cases/$case_name/$library.cc"
object_file="$output_dir/${case_name}_${library}.o"

if [ ! -f "$source_file" ]; then
  echo "unknown compile-time case: $case_name/$library" >&2
  exit 2
fi

mkdir -p "$output_dir"

: "${CXX:=c++}"
: "${CXXFLAGS:=-std=c++20 -O2}"

exec "$CXX" $CXXFLAGS \
  -I"$repo_root/include" \
  -I"$repo_root/bench/include" \
  -c "$source_file" \
  -o "$object_file"
