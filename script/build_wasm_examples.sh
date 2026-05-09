#!/bin/sh
# Compile cli20 example programs to WebAssembly for the interactive docs playground.
#
# Usage: build_wasm_examples.sh [examples_dir] [include_dir] [output_dir]
# Requires emcc (Emscripten) in PATH.

set -eu

EXAMPLES_DIR="${1:-examples}"
INCLUDE_DIR="${2:-include}"
OUT_DIR="${3:-docs/static/wasm}"

# Examples that cannot meaningfully run in a browser wasm environment
SKIP="module single_header unicode_echo"

mkdir -p "$OUT_DIR"

for src in "$EXAMPLES_DIR"/*.cc; do
  [ -f "$src" ] || continue
  name=$(basename "$src" .cc)

  skip=0
  for s in $SKIP; do
    [ "$name" = "$s" ] && skip=1 && break
  done
  [ "$skip" = "1" ] && continue

  echo "[wasm] $name"
  emcc -std=c++20 -I "$INCLUDE_DIR" \
    "$src" \
    -o "$OUT_DIR/${name}.js" \
    -sMODULARIZE=1 \
    -sEXPORT_ES6=1 \
    -sSINGLE_FILE=1 \
    -sEXPORTED_FUNCTIONS="['_main']" \
    -sEXPORTED_RUNTIME_METHODS="['callMain','FS']" \
    -sINVOKE_RUN=0 \
    -sFORCE_FILESYSTEM=1 \
    -sEXIT_RUNTIME=1 \
    -sALLOW_MEMORY_GROWTH=1 \
    -O2
done

echo "[wasm] done → $OUT_DIR"
