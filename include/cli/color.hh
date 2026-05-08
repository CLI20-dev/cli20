#pragma once

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <string_view>

namespace cli {

/**
 * @brief Controls whether ANSI escape codes are emitted by `format_help()`.
 *
 * - `auto_`  : emit codes only when stdout is connected to a TTY (default).
 * - `never`  : always produce plain text.
 * - `always` : always emit ANSI codes regardless of terminal type.
 */
enum class ColorMode { auto_, never, always };

/**
 * @brief Tag type passed to `format_help()` to request recursive sub-command
 * output.
 *
 * Pass the `cli::recurse_help` constant as an extra argument:
 * @code
 *   parser.format_help(cli::recurse_help);
 *   parser.format_help(cli::ColorMode::never, cli::recurse_help);
 * @endcode
 */
struct RecurseHelpTag {};

/** @brief Convenience instance of `RecurseHelpTag`. */
inline constexpr RecurseHelpTag recurse_help{};

namespace detail {

/**
 * @brief Wraps standard ANSI SGR (Select Graphic Rendition) sequences.
 *
 * Only the portable 8/16-color subset is used — no RGB/truecolor extensions.
 * All accessors return an empty `string_view` when `enabled` is false, so
 * callers can concatenate unconditionally without branches.
 */
struct AnsiStyle {
  /** @brief Whether ANSI codes should be emitted. */
  bool enabled;

  /**
   * @brief Returns the ANSI bold sequence, or an empty string when disabled.
   *
   * @return `"\033[1m"` if enabled, `""` otherwise.
   */
  [[nodiscard]] constexpr auto bold() const noexcept -> std::string_view {
    return enabled ? "\033[1m" : "";
  }

  /**
   * @brief Returns the ANSI underline sequence, or an empty string when
   * disabled.
   *
   * @return `"\033[4m"` if enabled, `""` otherwise.
   */
  [[nodiscard]] constexpr auto underline() const noexcept -> std::string_view {
    return enabled ? "\033[4m" : "";
  }

  /**
   * @brief Returns the ANSI reset sequence, or an empty string when disabled.
   *
   * @return `"\033[0m"` if enabled, `""` otherwise.
   */
  [[nodiscard]] constexpr auto reset() const noexcept -> std::string_view {
    return enabled ? "\033[0m" : "";
  }
};

/**
 * @brief Returns true when file descriptor 1 (stdout) is connected to a
 * terminal.
 *
 * Uses `_isatty` on Windows and `::isatty` on POSIX systems.
 *
 * @return `true` if stdout is a TTY, `false` otherwise.
 */
inline auto is_tty() noexcept -> bool {
#ifdef _WIN32
  return _isatty(1) != 0;
#else
  return ::isatty(STDOUT_FILENO) != 0;
#endif
}

/**
 * @brief Resolves a `ColorMode` to a concrete `AnsiStyle`.
 *
 * - `ColorMode::always` → `AnsiStyle{true}`
 * - `ColorMode::never`  → `AnsiStyle{false}`
 * - `ColorMode::auto_`  → `AnsiStyle{is_tty()}`
 *
 * @param mode The requested color mode.
 * @return An `AnsiStyle` with `enabled` set appropriately.
 */
inline auto resolve_color(ColorMode mode) noexcept -> AnsiStyle {
  const bool on =
      (mode == ColorMode::always) || (mode == ColorMode::auto_ && is_tty());
  return AnsiStyle{on};
}

}  // namespace detail
}  // namespace cli
