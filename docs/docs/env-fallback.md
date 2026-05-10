---
sidebar_position: 3
title: Environment Variable Fallback
---

# Environment Variable Fallback

Any option can fall back to an environment variable when it is absent from the command line. Set the `.env` field in the parameter bag:

```cpp
struct Args {
  cli::StringOption<"output", 'o'> output{{.env = "MYAPP_OUTPUT"}};
  cli::IntOption<"jobs", 'j'>     jobs{{.env = "MYAPP_JOBS"}};
  cli::Flag<"verbose", 'v'>       verbose{{.env = "MYAPP_VERBOSE"}};
};
```

If `--output` is not passed, the parser calls `getenv("MYAPP_OUTPUT")` and uses the result as the value. If the variable is also absent, the option remains unset.

## Precedence

Command-line values always take precedence. The env variable is only read when the option was not provided on the command line.

| Provided on CLI | Env var set | Result |
|---|---|---|
| yes | any | CLI value is used |
| no | yes | env value is used |
| no | no | default value (or missing-required error) |

## Type conversion and validation

The env value passes through the same action pipeline as a CLI value. Conversion and validation actions apply normally:

```cpp
cli::IntOption<"port"> port{{.env = "MYAPP_PORT"}};
```

If `MYAPP_PORT=not_a_number`, the parser returns an `invalid_value` error exactly as it would for `--port not_a_number`.

## Required options

An env variable satisfies a `required` option:

```cpp
cli::StringOption<"token"> token{{
    .presence = cli::required,
    .env      = "MYAPP_TOKEN",
}};
```

If neither `--token` nor `MYAPP_TOKEN` is present, the parser returns a `missing_required` error.

## Flags

For flags (`nargs::none`), any non-empty env value triggers the action, equivalent to passing the flag on the command line:

```cpp
cli::Flag<"verbose", 'v'> verbose{{.env = "MYAPP_VERBOSE"}};
// MYAPP_VERBOSE=1  →  verbose = true
```

The content of the variable is ignored; only its presence matters.
