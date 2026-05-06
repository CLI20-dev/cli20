# TODO

## Validator support

Add per-argument validation hooks that run after parsing, before the result is returned.

**Scope:**
- `ArgBase` / `Arg<T>`: `.validator(fn)` or constructor overload accepting a callable `T -> std::expected<void, std::string>`
- `PositionalArgument<T>`: same
- Validators run inside `parse()` after all values are assigned, collected as a single error pass
- Consider: range checks (`min`/`max` for numeric types), regex match for strings, enum-membership checks

**Example API sketch:**
```cpp
struct Args {
    IntArg<"port", 'p'> port{cli::validate([](int v) -> std::expected<void, std::string> {
        if (v < 1 || v > 65535) return std::unexpected("port must be 1–65535");
        return {};
    })};
    StrArg<"level"> level{cli::one_of({"debug", "info", "warn", "error"})};
};
```

---

## Constraints / conditional requirements

Express inter-argument dependencies at the type level or via runtime checks in `parse()`.

**Examples:**
- `--output` is required when `--format` is given
- `--depth` must not exceed `--jobs`
- Mutually exclusive groups: `--verbose` and `--quiet` cannot both be present
- "At least one of" groups: either `--file` or `--stdin` must be provided

**API sketch:**
```cpp
// At parse-call time (runtime constraint):
auto res = cli::parse<Args>(argc, argv);
if (res && res->format.provided() && !res->output.provided()) {
    return std::unexpected("--output is required when --format is given");
}

// Or declarative via a constraints() hook on the args struct:
struct Args {
    StrArg<"format"> format;
    StrArg<"output"> output;

    static auto constraints(const Args& a) -> std::expected<void, std::string> {
        if (a.format.provided() && !a.output.provided())
            return std::unexpected("--output is required when --format is given");
        return {};
    }
};
```

---

## Help / usage generation

**Done:**
- `formatHelp(ColorMode)` — usage line, Options / Positional arguments / Commands sections
- Per-argument description strings passed via constructor
- `cli::description` struct member for a detailed struct-level description shown after the usage line
- ANSI bold + underline section headers; bold option names; TTY auto-detection
- `HelpFlag<LongOpt, ShortOpt>` — special flag (default `--help` / `-h`) that bypasses parse errors so callers can unconditionally check `.provided()`
- `formatHelp(cli::recurseHelp)` — recursively appends each sub-command's help with a `─── name ───` separator
- User-controlled line wrapping: `\n` in a description indents continuation lines to the description column
- `Command<SubArgs, "name">` description falls back to `SubArgs::description` member when not set explicitly

**Remaining / nice-to-have:**
- Show a `(required)` marker (or similar) next to required options in the listing
- Show the default value for options that have one
- `--help` inside a sub-command (`prog build --help`) should render that sub-command's help

---

## Shell completion generation

Generate shell completion scripts (bash, zsh, fish) from the argument struct.

**Scope:**
- Complete long/short option names
- Complete sub-command names
- Value completions for known enums / file paths (via validator metadata)
- Output as a static string at build time or at runtime via `--completion <shell>`

---

## Short option combining

Support `-vxf` as shorthand for `-v -x -f` (flags only).

---

## `parse_known_args`

Return both the parsed `Arguments` and a `std::vector<std::string_view>` of unrecognized tokens, instead of erroring on unknown options.

---

## Sub-command required flag

Allow marking a `Command<>` field as required so that omitting the sub-command is an error:
```cpp
cli::Command<BuildArgs, "build"> build{cli::required};
```
