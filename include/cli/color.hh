#pragma once

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace cli {

// Controls whether ANSI escape codes are emitted by formatHelp().
//   auto_  : emit codes only when stdout is a TTY (default)
//   never  : always plain text
//   always : always emit codes regardless of terminal type
enum class ColorMode { auto_, never, always };

// Tag type passed to formatHelp() to request recursive sub-command output.
//   parser.formatHelp(cli::recurseHelp)               // auto color + recurse
//   parser.formatHelp(cli::ColorMode::never, cli::recurseHelp)  // no color
//   + recurse
struct RecurseHelpTag {};
inline constexpr RecurseHelpTag recurseHelp{};

namespace detail {

// Wraps standard ANSI SGR (Select Graphic Rendition) sequences.
// Only the portable 8/16-color subset is used — no RGB/truecolor extensions.
// All accessors return an empty string_view when disabled so callers can
// concatenate unconditionally without branches.
struct AnsiStyle {
  bool enabled;

  [[nodiscard]] constexpr auto bold() const noexcept -> std::string_view {
    return enabled ? "\033[1m" : "";
  }
  [[nodiscard]] constexpr auto underline() const noexcept -> std::string_view {
    return enabled ? "\033[4m" : "";
  }
  [[nodiscard]] constexpr auto reset() const noexcept -> std::string_view {
    return enabled ? "\033[0m" : "";
  }
};

// Returns true when file descriptor 1 (stdout) is connected to a terminal.
inline auto isTty() noexcept -> bool {
#ifdef _WIN32
  return _isatty(1) != 0;
#else
  return ::isatty(STDOUT_FILENO) != 0;
#endif
}

// Resolves a ColorMode to a concrete AnsiStyle.
inline auto resolveColor(ColorMode mode) noexcept -> AnsiStyle {
  const bool on =
      (mode == ColorMode::always) || (mode == ColorMode::auto_ && isTty());
  return AnsiStyle{on};
}

}  // namespace detail
}  // namespace cli
