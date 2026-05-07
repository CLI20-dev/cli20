# Contributing

Follow standard [GitHub Flow](https://docs.github.com/en/get-started/using-github/github-flow): fork, branch, PR against `main`.

## Formatting

Before opening a PR, format all code.

**Nix users:**
```sh
nix fmt
```

**Others:**
```sh
./format.sh
```

## Tests

All PRs must pass the test suite with clang-tidy enabled.

**Nix users:**
```sh
nix build
```

**Others:**
```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCXX_CLI20_ENABLE_TEST=ON \
  -DCXX_CLI20_ENABLE_CLANG_TIDY=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

> **Note:** clang-tidy requires a compiler that supports C++20 and a compatible version of clang-tidy in `PATH`.
