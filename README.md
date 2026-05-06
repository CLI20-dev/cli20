# argon

`argon` is a header-only C++ command-line parser where your struct is the schema.

No registration tables. No macros. No separate builder DSL. You write a plain aggregate, and `argon` turns it into a parser.

```cpp
struct Args {
  argon::FlagOption<"verbose", 'v'> verbose;
  argon::IntOption<"port", 'p'> port;
  argon::Positional<std::string, argon::nargs::one_or_more> files;
};
```

That is the interface.

## Why it is interesting

- The type of each field defines how it parses.
- Field order defines positional assignment.
- Subcommands are just nested structs.
- You can stay on the sugar API for common cases.
- You can drop to `ArgImpl` and `Action<...>` when you need custom conversion, validation, or packing.

## Installation

Copy `include/` into your project and add it to your include path.

```cmake
add_library(argon INTERFACE)
target_include_directories(argon INTERFACE path/to/argon/include)
target_compile_features(argon INTERFACE cxx_std_20)
```

## Quick Start

```cpp
#include <iostream>

#include "argon/argument.hh"
#include "argon/parser.hh"

struct BuildArgs {
  argon::FlagOption<"release", 'r'> release{
      {.help = "Build with optimizations"}};
  argon::IntOption<"jobs", 'j'> jobs{
      {.help = "Parallel jobs", .presence = argon::required}};
};

struct Args {
  argon::Description description{
      "A tiny build tool powered by argon."};

  argon::HelpFlag<> help{
      {.help = "Show help"}};

  argon::FlagOption<"verbose", 'v'> verbose{
      {.help = "Enable verbose logging"}};

  argon::StringOption<"output", 'o'> output{
      {.help = "Output path"}};

  argon::Positional<std::string, argon::nargs::one_or_more> inputs{
      {.help = "Input files", .presence = argon::required}};

  argon::Command<"build", BuildArgs> build{
      {.help = "Run the build subcommand"}};
};

int main(int argc, char* argv[]) {
  const auto args = argon::parseOrExit<Args>(argc, argv);
  if (args.verbose.value()) {
    std::cout << "verbose enabled\n";
  }

  for (const auto& input : args.inputs.value()) {
    std::cout << "input: " << input << '\n';
  }

  if (args.build.provided()) {
    std::cout << "jobs: " << *args.build.jobs.value() << '\n';
  }
}
```

Example CLI:

```text
tool --verbose src/a.cc src/b.cc build --jobs 8 --release
```

Built-in help is one field:

```cpp
argon::HelpFlag<> help;
```

That expands to `--help` / `-h`, prints generated help, and exits successfully. Together with `parseOrExit()`, a CLI can get built-in help with no error-handling boilerplate.

## The Sugar API

The main API lives in [`include/argon/argument.hh`](include/argon/argument.hh).

### Flags

```cpp
argon::FlagOption<"verbose", 'v'> verbose;
argon::HelpFlag<> help;
argon::HelpFlag<"usage", 'u'> usage;
argon::FlagArg<"dry-run"> dry_run;  // alias
```

Storage type: `bool` for ordinary flags. `HelpFlag` is signal-only and exits via its action pipeline.

### Scalar options

```cpp
argon::IntOption<"port", 'p'> port;
argon::StringOption<"config", 'c'> config;
argon::BoolOption<"color"> color;
argon::PathOption<"output"> output;
```

Storage type: `std::optional<T>`

`Option` and `Arg` names are interchangeable aliases:

```cpp
argon::IntOption<"port", 'p'> a;
argon::IntArg<"port", 'p'> b;
```

### List options

```cpp
argon::StringListOption<"include", 'I'> include_dirs;
argon::IntListArg<"ports"> ports;
```

Default `nargs` is `argon::nargs::one_or_more`. You can override it:

```cpp
argon::StringListOption<"feature", 'f', argon::nargs::zero_or_more> features;
argon::IntListOption<"pair", '\0', argon::nargs::exactly<2>> pair;
```

Storage type: `std::vector<T>`

### Positionals

```cpp
argon::StringPositional src;
argon::IntPositional count;
argon::Positional<std::string, argon::nargs::one_or_more> files;
```

If `nargs.max == 1`, the storage is `std::optional<T>`.

If `nargs.max != 1`, the storage is `std::vector<T>`.

### Built-in families

- `FlagOption`
- `StringOption`, `StringArg`, `StrOption`, `StrArg`
- `BoolOption`, `BoolArg`
- `IntOption`, `IntArg`
- `Int32Option`, `Int32Arg`
- `Int64Option`, `Int64Arg`
- `Uint32Option`, `Uint32Arg`
- `Uint64Option`, `Uint64Arg`
- `FloatOption`, `FloatArg`
- `DoubleOption`, `DoubleArg`
- `PathOption`, `PathArg`
- `StringListOption`, `StringListArg`, `StrListOption`, `StrListArg`
- `BoolListOption`, `BoolListArg`
- `IntListOption`, `IntListArg`
- `Int32ListOption`, `Int32ListArg`
- `Int64ListOption`, `Int64ListArg`
- `Uint32ListOption`, `Uint32ListArg`
- `Uint64ListOption`, `Uint64ListArg`
- `FloatListOption`, `FloatListArg`
- `DoubleListOption`, `DoubleListArg`
- `PathListOption`, `PathListArg`
- `StringPositional`, `StrPositional`
- `BoolPositional`
- `IntPositional`, `Int32Positional`, `Int64Positional`
- `Uint32Positional`, `Uint64Positional`
- `FloatPositional`, `DoublePositional`
- `PathPositional`

## `nargs`

`argon` ships named `Nargs` constants:

```cpp
argon::nargs::none
argon::nargs::one
argon::nargs::zero_or_one
argon::nargs::zero_or_more
argon::nargs::one_or_more
argon::nargs::exactly<3>
argon::nargs::between<2, 5>
```

## Presence

Use `argon::required` and `argon::optional`:

```cpp
argon::StringOption<"config"> config{
    {.help = "Path to config", .presence = argon::required}};
```

If a required option or positional is missing, `parse()` returns `missing_required`.

## Subcommands

Subcommands are nested parsers.

```cpp
struct ServeArgs {
  argon::IntOption<"port", 'p'> port{
      {.presence = argon::required}};
};

struct Args {
  argon::Command<"serve", ServeArgs> serve{
      {.help = "Start the HTTP server"}};
};
```

Usage:

```text
app serve --port 8080
```

Check whether the subcommand was used:

```cpp
if (result.value.serve.provided()) {
  std::cout << *result.value.serve.port.value() << '\n';
}
```

## Going Beyond Sugar

The sugar aliases are intentionally simple. When you need stronger behavior, use `ArgImpl` or `PositionalImpl` directly.

Example: parse a path, require it to exist, require it to be a regular file.

```cpp
argon::ArgImpl<
    "config", 'c', argon::nargs::one,
    argon::Action<
        argon::conversion::path,
        argon::validation::exists,
        argon::validation::is_regular_file,
        argon::pack::set_once>{}>
    config{{.help = "Configuration file"}};
```

The `Action` pipeline is split into three layers:

- `conversion::*`
- `validation::*`
- `pack::*`

This gives you a compact default API without closing off advanced composition.

## Parse Result

`argon::parse<T>()` and `argon::Parser<T>::parse()` return `ParseResult<T>`.

```cpp
auto result = argon::parse<Args>(argc, argv);
if (!result) {
  std::cerr << result.error.message() << '\n';
  return 1;
}
```

Useful pieces:

- `result.value`
- `result.error.code`
- `result.error.subject`
- `result.error.detail`
- `result.error.position`
- `result.error.message()`

Each argument object exposes:

- `arg.value()`
- `arg.provided()`

## Current Status

What already works well:

- tokenization
- scalar options
- list options
- positionals
- subcommands
- conversion / validation / pack action pipelines

What this project is good at:

- small tools
- internal CLIs
- typed command schemas
- projects that want compile-time structure instead of runtime registration

## Examples

- [`apps/example.cc`](apps/example.cc)
- [`apps/readme_example.cc`](apps/readme_example.cc)
- [`concept.cc`](concept.cc)

## Development

```bash
cmake -S . -B build -DCXX_ARGON_ENABLE_TEST=ON
cmake --build build
ctest --test-dir build --output-on-failure
```
