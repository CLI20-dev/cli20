#pragma once

#include <cstdio>
#include <cstdlib>

#include "cli/parser.hh"

namespace cli {

/**
 * @brief Parses `argc`/`argv` and exits the process on failure.
 *
 * This overload writes help/exit-success messages to `out` and errors to `err`
 * using stdio. Include this header when the FILE destinations must be
 * customized; `parser.hh` provides the simpler stdout/stderr fixed version.
 */
template <ArgumentSpec T>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
auto parse_or_exit(int argc, char* argv[], FILE* out, FILE* err) -> T {
  auto result = parse<T>(argc, argv);
  if (!result) {
    if (const auto message = result.error.message(); !message.empty()) {
      detail::write_message(result.error.use_stdout() ? out : err, message);
    }
    std::exit(result.error.exit_code());
  }
  return std::move(result.value);
}

}  // namespace cli
