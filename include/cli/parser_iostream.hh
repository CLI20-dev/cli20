#pragma once

#include <cstdlib>
#include <ostream>

#include "cli/parser.hh"

namespace cli {

/**
 * @brief Parses `argc`/`argv` and exits the process on failure.
 *
 * This overload writes help/exit-success messages to `out` and errors to `err`
 * using iostreams. Include this header only when stream-based output
 * customization is needed.
 */
template <ArgumentSpec T>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
auto parse_or_exit(int argc, char* argv[], std::ostream& out, std::ostream& err)
    -> T {
  auto result = parse<T>(argc, argv);
  if (!result) {
    if (const auto message = result.error.message(); !message.empty()) {
      auto& stream = result.error.use_stdout() ? out : err;
      stream << message << '\n';
    }
    std::exit(result.error.exit_code());
  }
  return std::move(result.value);
}

}  // namespace cli
