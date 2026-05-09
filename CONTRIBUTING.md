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

## Documentation

To preview the documentation site locally:

```sh
cd docs
npm ci
npm run start
```

To verify the production build:

```sh
cd docs
npm ci
npm run build
```

## Single header

`single_header/cli20.hh` is a build artifact — it is not tracked in git.

It is generated automatically by CMake as part of the build.

The generated file is uploaded as a release asset on each tagged release.
