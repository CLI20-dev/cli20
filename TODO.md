# TODO

- [x] `validate()` — post-parse validation hook on the parsed struct
- [ ] `visit_subcommand()` — visitor dispatch over matched subcommands
- [x] `range` / `choice` — built-in validation constraints
- [x] pipeline `action` types — validate action types and show clear error messages
- [x] env fallback — populate unprovided options from environment variables
- [ ] completion — shell completion script generation
- [x] short cluster / attached value — `-xvf`, `-ofile`
- [ ] `nargs::rest` — consume all remaining tokens as values (`min == max == -1`), disabling further option/subcommand detection for external-command style tails
