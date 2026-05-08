---
sidebar_position: 2
title: Tutorial
---

# Tutorial

This tutorial walks through all the major features of cli20, from simple flags to custom action pipelines.

## Flags

A flag is a boolean option that is `true` when present on the command line.

```cpp
cli::Flag<"verbose", 'v'> verbose;   // --verbose / -v, stores bool
cli::Help<> help;                     // --help / -h, print formatted help and exit
cli::Help<"usage", 'u'> usage;        // custom long name and short name
```

Flags store their value as `bool`. Access it with `.value()`:

```cpp
if (args.verbose.value()) {
  std::cout << "verbose mode\n";
}
```

## Options

Options take a value from the command line. cli20 provides a generic form and convenience aliases for common types.

### Generic form

```cpp
cli::Option<int, "port", 'p'>              port;
cli::ListOption<std::string, "include", 'I'> include_dirs;
```

### Convenience aliases

| Alias | Equivalent |
|---|---|
| `cli::StringOption<"name", 'n'>` | `cli::Option<std::string, "name", 'n'>` |
| `cli::IntOption<"count", 'c'>` | `cli::Option<int, "count", 'c'>` |
| `cli::DoubleOption<"ratio">` | `cli::Option<double, "ratio">` |
| `cli::PathOption<"output">` | `cli::Option<std::filesystem::path, "output">` |

Scalar options store `std::optional<T>`. List options store `std::vector<T>`.

```cpp
if (args.output.value()) {
  std::cout << "output: " << *args.output.value() << '\n';
}
```

## Positionals

Positional arguments are matched by position rather than name.

```cpp
cli::Positional<std::string>                         src;   // optional<string>
cli::Positional<std::string, cli::nargs::one_or_more> files; // vector<string>
cli::Positional<int>                                 count;
```

If `nargs.max == 1`, the storage type is `std::optional<T>`.  
If `nargs.max != 1`, the storage type is `std::vector<T>`.

```cpp
for (const auto& f : args.files.value()) {
  std::cout << "file: " << f << '\n';
}
```

## Built-in Help

Add a single field and help is wired up automatically:

```cpp
cli::Help<> help;
```

This expands to `--help` / `-h`. When the flag is present, cli20 prints a formatted help message and exits with success. Combined with `cli::parse_or_exit()`, no additional boilerplate is required.

For explicit control you can use `Arg` directly with an action pipeline:

```cpp
cli::Arg<
    "help", 'h',
    cli::action::print_help | cli::action::exit_success,
    cli::nargs::none>
    help;
```

## Subcommands

Subcommands are typed schemas nested inside a parent schema.

```cpp
struct BuildArgs {
  cli::Flag<"release", 'r'> release;
  cli::IntOption<"jobs", 'j'> jobs;
};

struct Args {
  cli::Command<"build", BuildArgs> build;
};
```

Check whether the subcommand was provided with `.provided()`:

```cpp
if (args.build.provided()) {
  std::cout << "jobs: " << *args.build.jobs.value() << '\n';
}
```

Subcommands are recursive — a `BuildArgs` can itself contain `cli::Command<...>` fields.

## Quick Start — full example

```cpp
#include <iostream>

#include "cli/argument.hh"
#include "cli/parser.hh"

struct BuildArgs {
  cli::Flag<"release", 'r'> release{
      {.help = "Build with optimizations"}};
  cli::IntOption<"jobs", 'j'> jobs{
      {.help = "Parallel jobs", .presence = cli::required}};
};

struct Args {
  cli::Description description{"A tiny build tool powered by cli20."};

  cli::Help<> help{{.help = "Show help"}};
  cli::Flag<"verbose", 'v'> verbose{{.help = "Enable verbose logging"}};
  cli::StringOption<"output", 'o'> output{{.help = "Output path"}};

  cli::Positional<std::string, cli::nargs::one_or_more> inputs{
      {.help = "Input files", .presence = cli::required}};

  cli::Command<"build", BuildArgs> build{
      {.help = "Run the build subcommand"}};
};

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parse_or_exit<Args>(argc, argv);

  if (args.verbose.value()) {
    std::cout << "verbose enabled\n";
  }

  for (const auto& input : args.inputs.value()) {
    std::cout << "input: " << input << '\n';
  }

  if (args.build.provided()) {
    std::cout << "jobs: " << *args.build.jobs.value() << '\n';
  }

  return 0;
}
```

## Going Beyond Sugar — action pipelines

When the sugar API is not enough, use `Arg` directly with a custom action pipeline.

Actions are composed with `|`:

```cpp
cli::Arg<
    "config", 'c',
    cli::conversion::path
        | cli::validation::exists
        | cli::validation::is_regular_file
        | cli::pack::set_once>
    config{{.help = "Configuration file"}};
```

Each `|` appends a step to the pipeline. The explicit `cli::Action<...>` template form is equivalent:

```cpp
cli::Arg<
    "config", 'c',
    cli::Action<
        cli::conversion::path,
        cli::validation::exists,
        cli::validation::is_regular_file,
        cli::pack::set_once>{}>
    config{{.help = "Configuration file"}};
```

The pipeline is split into three conceptual layers:

| Layer | Namespace | Purpose |
|---|---|---|
| Conversion | `cli::conversion::*` | Parse the raw string into a typed value |
| Validation | `cli::validation::*` | Reject out-of-range or otherwise invalid values |
| Packing | `cli::pack::*` | Store the final value into the field |

Use the sugar API for the common case and drop down to `Arg` only when you need stronger validation or custom conversion behavior.
