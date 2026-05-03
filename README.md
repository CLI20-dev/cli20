# argon

A header-only C++26 argument parsing library. The argument struct you write **is** the schema — no registration, no macros.

## Installation

Copy the `include/` directory into your project and add it to your include path.

```cmake
target_include_directories(your_target PRIVATE path/to/argon/include)
target_compile_features(your_target PRIVATE cxx_std_26)
```

## Quick start

```cpp
#include "argon/arithmetic_argument.hh"
#include "argon/flag_argument.hh"
#include "argon/parser.hh"
#include "argon/string_argument.hh"

struct Args {
    argon::FlagArg<"help",    'h'> help   {"Show this help message"};
    argon::FlagArg<"verbose", 'v'> verbose{"Enable verbose output"};
    argon::IntArg<"count",    'n'> count  {"Number of iterations"};
    argon::StrArg<"output",   'o'> output {"Output file path"};
};

int main(int argc, char** argv) {
    argon::Parser<Args> parser;
    auto res = parser.parse(argc, argv);

    if (!res) {
        std::cerr << "error: " << res.error().message() << '\n';
        std::cerr << parser.formatHelp();
        return 1;
    }
    if (res->help.provided()) {
        std::cout << parser.formatHelp();
        return 0;
    }
    if (res->verbose.provided()) { /* ... */ }
    if (res->count.provided())   { int n = res->count.value(); }
}
```

## The struct IS the schema

Every field in your struct becomes one argument. The type of the field determines how the token is parsed. Field order determines positional assignment order.

## Descriptions

Pass a string literal as the first constructor argument to document any argument. Descriptions appear in `formatHelp()` output.

```cpp
struct Args {
    argon::FlagArg<"verbose", 'v'> verbose{"Enable verbose output"};
    argon::IntArg<"count",    'n'> count  {"Number of iterations"};
    argon::StrArg<"remote",   'r'> remote {argon::required, "Remote name (required)"};
    argon::IntPositional           port   {"Port number to connect to"};
};
```

Constructor variants:

| Usage | Description | Required? |
|-------|-------------|-----------|
| `Arg<...> x;` | no description | optional |
| `Arg<...> x{"description"};` | description | optional |
| `Arg<...> x{argon::required};` | no description | required |
| `Arg<...> x{argon::required, "description"};` | description | required |

## Generating help text

```cpp
argon::Parser<Args> parser;
parser.parse(argc, argv);            // sets the program name from argv[0]

std::cout << parser.formatHelp();    // auto-detect TTY for color
std::cout << parser.formatHelp(argon::ColorMode::never);   // plain text
std::cout << parser.formatHelp(argon::ColorMode::always);  // always ANSI
```

Example output:

```
Usage: example [options] [command]

Options:
  -h, --help             Show this help message
  -v, --verbose          Enable verbose output
  -n, --count <int>      Number of iterations
  -o, --output <string>  Output file path

Commands:
  build                  Compile the project
  push                   Push commits to a remote
```

### Color modes

| `ColorMode` | Behaviour |
|-------------|-----------|
| `auto_` (default) | ANSI bold/underline when stdout is a TTY |
| `never` | Plain text, no escape codes |
| `always` | Always emit ANSI codes |

Only the portable 8/16-color SGR sequences are used (`\033[1m` bold, `\033[4m` underline, `\033[0m` reset). No RGB/truecolor extensions.

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

All options accept `--name value`, `-n value`, and `--name=value`. Short option is optional.

### List options (one or more values)

| Type | C++ type |
|------|----------|
| `IntListArg<"name">` | `std::vector<int>` |
| `StrListArg<"name">` | `std::vector<std::string>` |
| `BoolListArg<"name">` | `std::vector<bool>` |
| *(other `*ListArg` variants follow the same pattern)* | |

Collect multiple whitespace-separated values: `--names alice bob charlie`.  
Control the count with `nargs`:

```cpp
argon::StrListArg<"feature", 'f'> features{argon::nargs::zero_or_more, "Features to enable"};
argon::IntListArg<"ports">        ports{argon::nargs::one_or_more};
argon::IntListArg<"range">        range{argon::nargs::exactly<2>};
```

### Flags (no value)

```cpp
argon::FlagArg<"verbose", 'v'> verbose{"Enable verbose output"};
// usage: --verbose / -v
// result->verbose.provided() == true if present
```

### Positional arguments

Positionals are assigned left-to-right in field declaration order.

```cpp
argon::StrPositional src{"Source file"};   // first bare word
argon::StrPositional dst{"Destination"};   // second bare word
argon::IntPosArg     count{"Item count"};  // third, parsed as int
```

| Type | C++ type | Header |
|------|----------|--------|
| `StrPositional` / `StrPosArg` | `std::string` | `string_argument.hh` |
| `IntPositional` / `IntPosArg` | `int` | `arithmetic_argument.hh` |
| `Int64PosArg`, `Uint32PosArg`, … | fixed-width int | `arithmetic_argument.hh` |
| `FloatPosArg`, `DoublePosArg` | float/double | `arithmetic_argument.hh` |
| `BoolPositional` / `BoolPosArg` | `bool` (`true`/`false`) | `bool_argument.hh` |

### Required options

```cpp
argon::StrArg<"remote", 'r'> remote{argon::required, "Remote name"};
// parse() returns an error if --remote is not provided
```

### Sub-commands

Nest a struct inside `Command<SubArgs, "name">`. Everything from the command token onwards is parsed by a recursive sub-parser — top-level options must appear **before** the command name.

```cpp
struct BuildArgs {
    argon::StrArg<"target", 't'> target{"Build target"};
    argon::FlagArg<"dry-run">    dry_run{"Simulate without building"};
};

struct Args {
    argon::FlagArg<"verbose", 'v'>       verbose{"Verbose output"};
    argon::Command<BuildArgs, "build">   build{"Compile the project"};
    argon::Command<PushArgs,  "push">    push{"Push to remote"};
};

// usage: prog --verbose build --target release
if (res->build.provided()) {
    std::cout << res->build.target.value();
}
```

### `--` end-of-options separator

Everything after a bare `--` token is treated as a positional, even if it looks like an option:

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

## Argument API

Every argument field exposes:

```cpp
arg.provided()       // bool  — was this argument given on the command line?
arg.value()          // const T& — parsed value (default-constructed if not provided)
arg.description()    // std::string_view — the description string (empty if none)
arg.isRequired()     // bool  — was this declared required?
arg.occurrenceCount() // std::size_t — how many times the option was seen
```

## Error handling

`Parser::parse` returns `std::expected<Args, ParseError>`. On failure:

```cpp
auto res = parser.parse(argc, argv);
if (!res) {
    std::cerr << "error: " << res.error().message() << '\n';
    std::cerr << parser.formatHelp(argon::ColorMode::never);
    return 1;
}
```

`ParseError` fields:

```cpp
res.error().code     // ErrorCode enum value
res.error().subject  // option or argument name involved
res.error().detail   // human-readable explanation
res.error().message() // formatted string combining all fields
```

## Requirements

- C++26 (structured binding packs, `std::expected`)
- Clang 19+ with libc++ (required for `std::from_chars` on floating-point types)
