# argon

A header-only C++26 argument parsing library. The argument struct you write **is** the schema — no registration, no macros.

## Installation

Copy the `include/` directory into your project and add it to your include path. That's it.

```cmake
target_include_directories(your_target PRIVATE path/to/argon/include)
target_compile_features(your_target PRIVATE cxx_std_26)
```

## Quick start

```cpp
#include "argon/arithmetic_argument.hh"
#include "argon/parser.hh"
#include "argon/string_argument.hh"

struct Args {
    argon::FlagArg<"verbose", 'v'> verbose;
    argon::IntArg<"count", 'n'>    count;
    argon::StrArg<"output", 'o'>   output;
};

int main(int argc, char** argv) {
    argon::Parser<Args> parser;
    auto res = parser.parse(argc, argv);
    if (!res) {
        std::cerr << "error: " << res.error() << '\n';
        return 1;
    }
    if (res->verbose.seen()) { /* ... */ }
    if (res->count.seen())   { int n = res->count.value(); }
}
```

## The struct IS the schema

Every field in your struct becomes one argument. The type of the field determines how the token is parsed. Field order determines positional assignment order.

## Argument types

### Options (take a value)

| Type | C++ type | Header |
|------|----------|--------|
| `IntArg<"name", 'n'>` | `int` | `arithmetic_argument.hh` |
| `Int32Arg<"name">` | `int32_t` | `arithmetic_argument.hh` |
| `Int64Arg<"name">` | `int64_t` | `arithmetic_argument.hh` |
| `Uint32Arg<"name">` | `uint32_t` | `arithmetic_argument.hh` |
| `Uint64Arg<"name">` | `uint64_t` | `arithmetic_argument.hh` |
| `FloatArg<"name">` | `float` | `arithmetic_argument.hh` |
| `DoubleArg<"name">` | `double` | `arithmetic_argument.hh` |
| `StrArg<"name", 'n'>` | `std::string` | `string_argument.hh` |
| `BoolArg<"name">` | `bool` (`true`/`false` only) | `bool_argument.hh` |

All options accept `--name value`, `-n value`, and `--name=value`. Short option (`'n'`) is optional — omit it or leave as `'\0'`.

### List options (take one or more values)

| Type | C++ type |
|------|----------|
| `IntListArg<"name">` | `std::vector<int>` |
| `StrListArg<"name">` | `std::vector<std::string>` |
| `BoolListArg<"name">` | `std::vector<bool>` |
| *(other `*ListArg` variants follow the same pattern)* | |

Collect multiple whitespace-separated values: `--names alice bob charlie`.  
Control the count with `nargs`:

```cpp
argon::StrListArg<"feature", 'f'> features{argon::nargs::zero_or_more};
argon::IntListArg<"ports">        ports{argon::nargs::one_or_more};
argon::IntListArg<"range">        range{argon::nargs::exactly<2>};
```

### Flags (no value)

```cpp
argon::FlagArg<"verbose", 'v'> verbose;
// usage: --verbose / -v
// result->verbose.seen() == true if provided
```

### Positional arguments

Positionals are assigned left-to-right in field declaration order.

```cpp
argon::StrPositional  src;   // first bare word
argon::StrPositional  dst;   // second bare word
argon::IntPosArg      count; // third bare word, parsed as int
```

| Type | C++ type | Header |
|------|----------|--------|
| `StrPositional` / `StrPosArg` | `std::string` | `string_argument.hh` |
| `IntPositional` / `IntPosArg` | `int` | `arithmetic_positional.hh` |
| `Int64PosArg`, `Uint32PosArg`, … | fixed-width int | `arithmetic_positional.hh` |
| `FloatPosArg`, `DoublePosArg` | float/double | `arithmetic_positional.hh` |
| `BoolPositional` / `BoolPosArg` | `bool` (`true`/`false`) | `bool_argument.hh` |

### Required options

```cpp
argon::StrArg<"remote", 'r'> remote{argon::required};
// parse() returns an error if --remote is not provided
```

### Sub-commands

Nest a struct inside `Command<SubArgs, "name">`. Everything from the command token onwards is parsed by a recursive sub-parser — top-level options must appear **before** the command name.

```cpp
struct BuildArgs {
    argon::StrArg<"target", 't'> target;
    argon::FlagArg<"dry-run">    dry_run;
};

struct Args {
    argon::FlagArg<"verbose", 'v'>   verbose;
    argon::Command<BuildArgs, "build"> build;
    argon::Command<PushArgs,  "push">  push;
};

// usage: prog --verbose build --target release
if (res->build.seen()) {
    auto& b = res->build.args;
    // b.target.value(), b.dry_run.seen(), ...
}
```

### `--` end-of-options separator

Everything after a bare `--` token is treated as a positional argument, even if it looks like an option name:

```
prog --output out.txt -- --not-an-option
```

## `nargs` reference

| Constant | Meaning |
|----------|---------|
| `nargs::one` | exactly 1 (default for single-value options) |
| `nargs::none` | 0 (default for flags) |
| `nargs::optional` | 0 or 1 |
| `nargs::zero_or_more` | 0 … ∞ |
| `nargs::one_or_more` | 1 … ∞ |
| `nargs::exactly<N>` | exactly N |
| `nargs::between<Min, Max>` | Min … Max |

## All argument methods

Every argument field exposes:

```cpp
arg.seen()   // bool — was this argument provided on the command line?
arg.value()  // const T& — the parsed value (default-initialized if not seen)
```

Options also expose:
```cpp
arg.occurrenceCount()  // std::size_t — how many times seen (usually 0 or 1)
```

## Full example

See [`apps/example.cc`](apps/example.cc) for a complete program with two sub-commands, list options, flags, required options, and `--` handling.

```
./example --verbose -j 4 build --target release --feature sse4 avx2
./example push --remote origin --force
./example --config myconf.toml push -r upstream --depth 3
./example build -- --not-a-flag
```

## Error handling

`Parser::parse` returns `std::expected<Args, std::string>`. On failure the error string describes the problem:

```
error: required option '--remote' was not provided
error: option '--count' requires at least 1 argument(s), but got 0
error: option '--count' specified multiple times
```

## Requirements

- C++26 (structured binding packs, `std::expected`)
- Clang 19+ with libc++ (for `std::from_chars` on floating-point types)
