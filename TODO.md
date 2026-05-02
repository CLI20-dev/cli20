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
    IntArg<"port", 'p'> port{argon::validate([](int v) -> std::expected<void, std::string> {
        if (v < 1 || v > 65535) return std::unexpected("port must be 1–65535");
        return {};
    })};
    StrArg<"level"> level{argon::one_of({"debug", "info", "warn", "error"})};
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
auto res = argon::parse<Args>(argc, argv);
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

Auto-generate a `--help` / `-h` usage string from the argument struct.

**Scope:**
- Per-argument description strings as optional metadata (e.g. `IntArg<"port"> port{argon::help("port to listen on")}`)
- Top-level usage line: `usage: prog [OPTIONS] <COMMAND>`
- Options section listing long/short names, value type, default, required marker
- Sub-command section listing command names with one-line descriptions
- Automatic `-h` / `--help` flag injection (or opt-out via parser config)

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
argon::Command<BuildArgs, "build"> build{argon::required};
```
