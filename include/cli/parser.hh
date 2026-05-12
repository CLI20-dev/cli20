#pragma once

// #include <expected>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <string>
#include <tuple>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "cli/argument.hh"
#include "cli/constraint.hh"
#include "cli/error.hh"
#include "cli/help.hh"

namespace cli {

namespace detail {

inline auto write_message(FILE* file, std::string_view message) -> void {
  if (file == nullptr || message.empty()) return;
  (void)std::fwrite(message.data(), 1, message.size(), file);
  (void)std::fwrite("\n", 1, 1, file);
}

inline auto value_count_message(int required, int provided) -> std::string {
  return "option requires at least " + std::to_string(required) +
         " value(s), but " + std::to_string(provided) + " provided";
}

inline auto inline_value_count_message(int required) -> std::string {
  return "option requires at least " + std::to_string(required) +
         " value(s), but inline separator provides only 1";
}

}  // namespace detail

#ifdef _WIN32
namespace detail {
/**
 * @brief Converts a wide-character string to UTF-8.
 *
 * Uses `WideCharToMultiByte` with code page `CP_UTF8`. Returns an empty string
 * for a null pointer, an empty string, or a conversion failure.
 *
 * @param wide Null-terminated wide-character string, or `nullptr`.
 * @return The UTF-8 encoded equivalent, or `""` on failure.
 */
inline auto wide_to_utf8(const wchar_t* wide) -> std::string {
  if (!wide) return {};
  constexpr DWORD flags = WC_ERR_INVALID_CHARS;
  int len = WideCharToMultiByte(CP_UTF8, flags, wide, -1, nullptr, 0, nullptr,
                                nullptr);
  if (len <= 1) return {};
  std::string result(static_cast<std::size_t>(len) - 1, '\0');
  const int written = WideCharToMultiByte(CP_UTF8, flags, wide, -1,
                                          result.data(), len, nullptr, nullptr);
  if (written != len) return {};
  return result;
}
}  // namespace detail

/**
 * @brief Owns UTF-8 strings converted from a Windows UTF-16 `argv`.
 *
 * The pointer array returned by `data()` is compatible with APIs that accept
 * `char* argv[]`, including `cli::parse(argc, argv)`.
 */
struct Utf8Argv {
  std::vector<std::string> storage;
  std::vector<char*> pointers;

  [[nodiscard]] auto size() const -> int {
    return static_cast<int>(pointers.size());
  }

  [[nodiscard]] auto data() -> char** { return pointers.data(); }
};

/**
 * @brief Converts a Windows UTF-16 `argv` to an owned UTF-8 `argv` wrapper.
 *
 * This is intentionally separate from parsing. `wmain` entry points can convert
 * once and then call the ordinary UTF-8 parser overload:
 *
 * @code
 * auto argv_utf8 = cli::utf16_to_utf8(argc, argv);
 * auto args = cli::parse_or_exit<Args>(argv_utf8.size(), argv_utf8.data());
 * @endcode
 */
inline auto utf16_to_utf8(int argc, wchar_t* argv[]) -> Utf8Argv {
  Utf8Argv result;
  result.storage.reserve(static_cast<std::size_t>(argc));
  result.pointers.reserve(static_cast<std::size_t>(argc));
  for (int i = 0; i < argc; ++i) {
    result.storage.emplace_back(detail::wide_to_utf8(argv[i]));
  }
  for (std::string& arg : result.storage) {
    result.pointers.push_back(arg.data());
  }
  return result;
}
#endif

/**
 * @brief Classifies a single token produced by `tokenize()`.
 *
 * - `option`     : An option name token (e.g. `"--verbose"`, `"-v"`).
 * - `value`      : A value token associated with the preceding option.
 * - `positional` : A bare positional argument.
 * - `command`    : A subcommand name; subsequent tokens belong to the
 * subcommand.
 */
enum class TokenType : uint8_t {
  option,
  value,
  positional,
  command,
};

static constexpr std::uint8_t kNoPrefix = 0xff;
static constexpr std::uint8_t kShortPrefixBit = 0x80;
static constexpr std::uint8_t kPrefixIndexMask = 0x7f;

static constexpr auto no_prefix() -> std::uint8_t { return kNoPrefix; }

static constexpr auto short_prefix() -> std::uint8_t { return kShortPrefixBit; }

static constexpr auto long_prefix(std::uint8_t i) -> std::uint8_t {
  return i & kPrefixIndexMask;
}

static constexpr auto has_prefix(std::uint8_t p) -> bool {
  return p != kNoPrefix;
}

static constexpr auto is_short_prefix(std::uint8_t p) -> bool {
  return p != kNoPrefix && (p & kShortPrefixBit);
}

/**
 * @brief A single token produced by `tokenize()`.
 */
struct Token {
  std::string_view text;   ///< For option tokens: the bare name without prefix
                           ///< (e.g. `"verbose"` for `--verbose`, `"v"` for
                           ///< `-v`). For value/positional/command tokens: the
                           ///< raw text from the args span.
  std::uint16_t position;  ///< Zero-based index into the original `args` span.
  std::uint8_t prefix;  // 0xff = none, high bit = short prefix, low 7bit = long
                        // prefix index
  TokenType type;       ///< Classification of this token.
};

/**
 * @brief The result returned by `tokenize()`.
 *
 * On success, `tokens` contains the ordered token sequence and `error` is in
 * the default (no-error) state. On failure, `tokens` may be partial and
 * `error` describes the first problem encountered.
 */
struct TokenizeResult {
  template <class T, std::size_t N>

  struct SmallVector {
    std::array<T, N> stack{};
    std::vector<T> heap;
    std::size_t size_value{};

    auto push_back(T value) -> void {
      if (size_value < N && heap.empty()) {
        stack[size_value++] = std::move(value);
        return;
      }

      if (heap.empty()) {
        heap.reserve(N * 2);
        for (std::size_t i = 0; i < size_value; ++i) {
          heap.push_back(std::move(stack[i]));
        }
      }

      heap.push_back(std::move(value));
      ++size_value;
    }

    [[nodiscard]] auto size() const -> std::size_t { return size_value; }
    [[nodiscard]] auto empty() const -> bool { return size_value == 0; }

    auto operator[](std::size_t i) -> T& {
      return heap.empty() ? stack[i] : heap[i];
    }

    auto operator[](std::size_t i) const -> const T& {
      return heap.empty() ? stack[i] : heap[i];
    }

    [[nodiscard]] auto begin() const {
      return heap.empty() ? stack.data() : heap.data();
    }

    [[nodiscard]] auto end() const { return begin() + size_value; }

    [[nodiscard]] auto data() const {
      return heap.empty() ? stack.data() : heap.data();
    }
  };

  SmallVector<Token, 32> tokens;
  ParseError error{};

  /** @brief Returns `true` when tokenization succeeded. */
  [[nodiscard]]
  constexpr explicit operator bool() const noexcept {
    return !error.has_error();
  }

  /** @brief Returns `true` when tokenization succeeded. */
  [[nodiscard]]
  constexpr auto has_value() const noexcept -> bool {
    return !error.has_error();
  }

  /** @brief Returns `true` when tokenization failed. */
  [[nodiscard]]
  constexpr auto has_error() const noexcept -> bool {
    return error.has_error();
  }

  /**
   * @brief Sets the error and returns `*this` for method chaining.
   *
   * @param code    The error code.
   * @param pos     The zero-based index into the input `args` span where the
   *                error occurred. Higher-level parser code may apply an
   *                offset when translating this to a caller-visible argument
   *                index.
   * @param subject The option or token name that caused the error.
   * @param detail  Optional additional detail message.
   * @return Reference to `*this`.
   */
  auto fail(ErrorCode code, std::size_t pos, std::string subject,
            std::string detail = {}) -> TokenizeResult& {
    error = ParseError{
        .code = code,
        .kind = ErrorKind::parse,
        .position = static_cast<int>(pos),
        .subject = std::move(subject),
        .detail = std::move(detail),
    };
    return *this;
  }
};

/**
 * @brief Controls how `tokenize()` recognises option and separator syntax.
 *
 * The defaults implement GNU-style `--long` / `-s` argument conventions with
 * `=` as the inline value separator and `--` as the end-of-options marker.
 */
struct TokenizerConfig {
 private:
  static constexpr std::size_t max_option_prefixes = 4;

  std::array<std::string_view, max_option_prefixes> option_prefixes_{"--"};
  std::uint8_t option_prefix_count_{1};

  std::string_view short_option_prefix_{"-"};
  std::string_view end_of_options_separator_{"--"};
  char inline_value_separator_{'='};

 public:
  constexpr TokenizerConfig() = default;

  [[nodiscard]] constexpr auto option_prefixes() const
      -> std::span<const std::string_view> {
    return {option_prefixes_.data(), option_prefix_count_};
  }

  [[nodiscard]] constexpr auto short_option_prefix() const -> std::string_view {
    return short_option_prefix_;
  }

  [[nodiscard]] constexpr auto end_of_options_separator() const
      -> std::string_view {
    return end_of_options_separator_;
  }

  [[nodiscard]] constexpr auto inline_value_separator() const -> char {
    return inline_value_separator_;
  }

  constexpr auto set_option_prefixes(
      std::initializer_list<std::string_view> prefixes) -> void {
    option_prefix_count_ = 0;

    for (auto p : prefixes) {
      if (option_prefix_count_ >= max_option_prefixes) break;
      option_prefixes_[option_prefix_count_++] = p;
    }
  }

  [[nodiscard]] constexpr auto with_option_prefixes(
      std::initializer_list<std::string_view> prefixes) const
      -> TokenizerConfig {
    auto cfg = *this;
    cfg.set_option_prefixes(prefixes);
    return cfg;
  }

  [[nodiscard]] constexpr auto with_short_option_prefix(
      std::string_view prefix) const -> TokenizerConfig {
    auto cfg = *this;
    cfg.short_option_prefix_ = prefix;
    return cfg;
  }

  [[nodiscard]] constexpr auto with_end_of_options_separator(
      std::string_view separator) const -> TokenizerConfig {
    auto cfg = *this;
    cfg.end_of_options_separator_ = separator;
    return cfg;
  }

  [[nodiscard]] constexpr auto with_inline_value_separator(char separator) const
      -> TokenizerConfig {
    auto cfg = *this;
    cfg.inline_value_separator_ = separator;
    return cfg;
  }
};

/**
 * @brief Retrieves the matched prefix string for an option token.
 *
 * For a token with `prefix == kNoPrefix`, returns an empty view. For a short
 * prefix, returns `cfg.short_option_prefix`. For a long prefix, returns the
 * corresponding entry from `cfg.option_prefixes`.
 *
 * @param tok The token whose prefix to retrieve; must be of type `option`.
 * @param cfg The tokenizer configuration that was used to parse `tok`.
 * @return The matched prefix string as a view into `cfg`, or empty if no
 * prefix.
 */
inline auto token_prefix_view(const Token& tok, const TokenizerConfig& cfg)
    -> std::string_view {
  if (tok.prefix == kNoPrefix) return {};
  if (tok.prefix & kShortPrefixBit) return cfg.short_option_prefix();
  return cfg.option_prefixes()[tok.prefix & kPrefixIndexMask];
}

/**
 * @brief Converts a raw argument span into an ordered sequence of typed tokens.
 *
 * This is a pure lexical operation; it does not access or modify any parsed
 * result struct. It is used internally by `Parser::parse_body` but can also be
 * called standalone for custom parsing scenarios.
 *
 * **Token sequence rules:**
 * - An option token (`{option, "--opt", i}`) is followed by zero or more
 *   `{value, "...", j}` tokens greedily consumed up to `nargs.max`.
 * - `--opt=val` is split into `{option, "--opt", i}` + `{value, "val", i}`
 *   at the same position.
 * - Bare tokens that match a command name produce `{command, "cmd", i}` and
 *   tokenization stops immediately; `args[i+1:]` are the subcommand's tokens.
 * - After the end-of-options separator (`--`), all remaining tokens become
 *   `{positional, "...", i}` regardless of prefix.
 *
 * @param args          The argument span to tokenize (typically `argv[1:]`).
 * @param spec_map      Map from full option keys (e.g. `"--verbose"`, `"-v"`) to
 * their `Nargs`. Must NOT contain command names.
 * @param command_names Set of bare subcommand name strings (no prefix).
 * @param cfg           Tokenizer configuration (prefixes, separator character,
 * etc.).
 * @return A `TokenizeResult` containing the token list on success, or a
 * `ParseError` on failure.
 */
template <class OptionLookup, class CommandLookup>
inline auto tokenize(std::span<const std::string_view> args,
                     OptionLookup option_nargs, CommandLookup is_command,
                     const TokenizerConfig& cfg = {}) -> TokenizeResult {
  TokenizeResult result;

  // Returns the matched prefix (as a view into cfg.option_prefixes) or empty.
  const auto find_prefix = [&](std::string_view tok) -> std::uint8_t {
    for (std::uint8_t pi = 0;
         pi < static_cast<uint8_t>(cfg.option_prefixes().size()); ++pi) {
      const auto& p = cfg.option_prefixes()[pi];
      if (tok.size() > p.size() && tok.starts_with(p)) {
        return long_prefix(pi);
      }
    }

    if (tok.size() > cfg.short_option_prefix().size() &&
        tok.starts_with(cfg.short_option_prefix())) {
      return short_prefix();
    }

    return no_prefix();
  };

  const auto prefix_view = [&](std::uint8_t p) -> std::string_view {
    if (p == no_prefix()) return {};
    if (is_short_prefix(p)) return cfg.short_option_prefix();
    return cfg.option_prefixes()[p & kPrefixIndexMask];
  };

  const auto push = [&](TokenType type, std::string_view text, std::uint16_t pos,
                        std::uint8_t prefix = kNoPrefix) -> void {
    result.tokens.push_back({
        .text = text,
        .position = pos,
        .prefix = prefix,
        .type = type,
    });
  };

  bool post_separator = false;
  std::size_t i = 0;

  while (i < args.size()) {
    const std::string_view tok = args[i];

    // ── option parsing terminator ─────────────────────────────────────────
    if (!post_separator && tok == cfg.end_of_options_separator()) {
      post_separator = true;
      ++i;
      continue;
    }

    // ── after separator: everything is positional ─────────────────────────
    if (post_separator) {
      push(TokenType::positional, tok, i++);
      continue;
    }

    // ── command name ──────────────────────────────────────────────────────
    const auto prefix = find_prefix(tok);
    const auto prefix_sv = prefix_view(prefix);
    if (!has_prefix(prefix) && is_command(tok)) {
      push(TokenType::command, tok, i);
      break;  // args[i+1:] are the sub-command's responsibility
    }

    // ── option-looking token ──────────────────────────────────────────────
    if (has_prefix(prefix)) {
      const auto sep = cfg.inline_value_separator();
      const auto eq_pos = tok.find(sep);
      const bool has_inline_val = (eq_pos != std::string_view::npos);
      const std::string_view opt_sv =
          has_inline_val ? tok.substr(0, eq_pos) : tok;
      const std::string opt_name{opt_sv};
      const auto nargs_opt = option_nargs(opt_name);
      if (!nargs_opt) {
        // Short cluster (-xvf) or attached value (-ofile): only when the token
        // uses the short prefix and carries more than one character after it.
        const std::string_view body = opt_sv.substr(prefix_sv.size());
        if (is_short_prefix(prefix) && !has_inline_val && body.size() > 1) {
          bool advanced_i = false;
          const std::string_view char_prefix = tok.substr(0, prefix_sv.size());
          std::string char_full{char_prefix};
          char_full.push_back('\0');
          for (std::size_t k = 0; k < body.size(); ++k) {
            const std::string_view char_bare =
                tok.substr(prefix_sv.size() + k, 1);
            char_full.back() = body[k];
            const auto nargs_opt = option_nargs(char_full);
            if (!nargs_opt) {
              return result.fail(ErrorCode::unknown_option, i, char_full);
            }
            const Nargs nargs = *nargs_opt;
            const bool is_flag = (nargs.min == 0 && nargs.max == 0);
            push(TokenType::option, char_bare, i, prefix);
            if (!is_flag) {
              if (k + 1 < body.size()) {
                // First value is attached (rest of body).
                push(TokenType::value, tok.substr(prefix_sv.size() + k + 1), i);
                ++i;
                // Consume additional tokens for nargs > 1.
                int count = 1;
                while (i < args.size()) {
                  const std::string_view next = args[i];
                  if (next == cfg.end_of_options_separator()) break;
                  if (is_command(std::string(next))) break;
                  if (nargs.max != -1 && count >= nargs.max) break;
                  if (has_prefix(find_prefix(next))) {
                    const auto next_sep = next.find(sep);
                    const std::string next_opt{next_sep != std::string_view::npos
                                                   ? next.substr(0, next_sep)
                                                   : next};
                    if (option_nargs(next_opt).has_value()) break;
                    if (count >= 1 || nargs.min == 0) break;
                  }
                  push(TokenType::value, args[i], i);
                  ++i;
                  ++count;
                }
                if (count < nargs.min) {
                  return result.fail(
                      ErrorCode::missing_value, i, char_full,
                      detail::value_count_message(nargs.min, count));
                }
              } else {
                // Consume next token(s) greedily as values
                ++i;
                int count = 0;
                while (i < args.size()) {
                  const std::string_view next = args[i];
                  if (next == cfg.end_of_options_separator()) break;
                  if (is_command(std::string(next))) break;
                  if (nargs.max != -1 && count >= nargs.max) break;
                  if (has_prefix(find_prefix(next))) {
                    const auto next_sep = next.find(sep);
                    const std::string next_opt{next_sep != std::string_view::npos
                                                   ? next.substr(0, next_sep)
                                                   : next};
                    if (option_nargs(next_opt).has_value()) break;
                    if (count >= 1 || nargs.min == 0) break;
                  }
                  push(TokenType::value, args[i], i);
                  ++i;
                  ++count;
                }
                if (count < nargs.min) {
                  return result.fail(
                      ErrorCode::missing_value, i, char_full,
                      detail::value_count_message(nargs.min, count));
                }
              }
              advanced_i = true;
              break;
            }
          }
          if (!advanced_i) ++i;
          continue;
        }
        return result.fail(ErrorCode::unknown_option, i, opt_name);
      }

      const Nargs& nargs = *nargs_opt;
      const bool is_flag = (nargs.min == 0 && nargs.max == 0);

      // Store bare name (without prefix) in text; matched_prefix holds the
      // prefix.

      push(TokenType::option, opt_sv.substr(prefix_sv.size()), i, prefix);

      if (has_inline_val) {
        if (is_flag) {
          return result.fail(ErrorCode::invalid_value, i, opt_name,
                             "flag does not accept a value");
        }
        if (nargs.min > 1) {
          return result.fail(ErrorCode::missing_value, i, opt_name,
                             detail::inline_value_count_message(nargs.min));
        }
        push(TokenType::value, tok.substr(eq_pos + 1), i);
        ++i;
        continue;
      }

      // prefix-opt form: advance, then greedily consume value tokens
      ++i;

      if (is_flag) continue;

      int count = 0;
      while (i < args.size()) {
        const std::string_view next = args[i];

        if (next == cfg.end_of_options_separator()) break;
        if (is_command(std::string(next))) break;
        if (nargs.max != -1 && count >= nargs.max) break;

        if (has_prefix(find_prefix(next))) {
          const auto next_sep = next.find(sep);
          const std::string next_opt{next_sep != std::string_view::npos
                                         ? next.substr(0, next_sep)
                                         : next};
          if (option_nargs(next_opt).has_value()) break;

          // Unknown option-looking: consume only as first value when min > 0
          if (count >= 1 || nargs.min == 0) break;
        }

        push(TokenType::value, args[i], i);
        ++i;
        ++count;
      }

      if (count < nargs.min) {
        return result.fail(ErrorCode::missing_value, i, opt_name,
                           detail::value_count_message(nargs.min, count));
      }
      continue;
    }

    // ── bare token → positional ───────────────────────────────────────────
    push(TokenType::positional, tok, i++);
  }

  return result;
}

/**
 * @brief The result returned by `Parser::parse()` and the free `cli::parse()`
 * function.
 *
 * On success, `value` holds the fully populated argument specification struct
 * and `error` is in the default (no-error) state.  On failure, `error` describes
 * what went wrong and `value` is in a partially-initialised (unusable) state.
 *
 * Typical usage:
 * @code
 *   auto result = cli::parse<MyArgs>(argc, argv);
 *   if (!result) {
 *       std::cerr << result.error.message() << '\n';
 *       return 1;
 *   }
 *   // use result.value …
 * @endcode
 *
 * @tparam T The argument specification type.
 */
template <class T>
struct ParseResult {
  T value{};  ///< The parsed argument struct (valid only when `has_value()` is
              ///< true).
  ParseError
      error{};  ///< The error details (valid only when `has_error()` is true).

  /** @brief Returns `true` when parsing succeeded. */
  [[nodiscard]]
  constexpr explicit operator bool() const noexcept {
    return !error.has_error();
  }

  /** @brief Returns `true` when parsing succeeded. */
  [[nodiscard]]
  constexpr auto has_value() const noexcept -> bool {
    return !error.has_error();
  }

  /** @brief Returns `true` when parsing failed. */
  [[nodiscard]]
  constexpr auto has_error() const noexcept -> bool {
    return error.has_error();
  }

  /**
   * @brief Creates a success result holding `value`.
   *
   * @tparam U Type of the value (deduced).
   * @param value The parsed argument struct to store.
   * @return A successful `ParseResult<T>`.
   */
  template <class U>
  [[nodiscard]]
  static constexpr auto ok(U&& value) -> ParseResult<T> {
    return {
        .value = std::forward<U>(value),
        .error = {},
    };
  }

  /**
   * @brief Creates a failure result carrying `error`.
   *
   * @param error The error to store.
   * @return A failed `ParseResult<T>`.
   */
  [[nodiscard]]
  static constexpr auto fail(ParseError error) -> ParseResult<T> {
    return {
        .value = {},
        .error = std::move(error),
    };
  }
};

/**
 * @brief The main parsing engine for a typed argument specification.
 *
 * `Parser<T>` owns a `TokenizerConfig` and the program name used in help output.
 * It tokenizes the argument list, dispatches each token to the appropriate field
 * of `T`, validates required arguments, and fires `on_parse` callbacks.
 *
 * **Entry points:**
 * - `parse(argc, argv)` — standard C-style interface; sets program name from
 * `argv[0]`.
 * - `parse(span, first_index)` — span-based interface with optional offset.
 * - `parse(initial, ...)` — pre-initialised variants; useful for `BoundOption` /
 * `on_parse`.
 *
 * On Windows, use `cli::utf16_to_utf8(argc, argv)` in `wmain` and pass the
 * converted UTF-8 argv to the standard `char*` overload.
 *
 * @tparam T The argument specification type (must satisfy `ArgumentSpec`).
 */
template <ArgumentSpec T>
struct Parser {
  template <ArgumentSpec>
  friend struct Parser;

  /**
   * @brief Parses standard C-style `argc`/`argv`, using `argv[0]` as the program
   * name.
   *
   * @param argc Argument count (includes program name at index 0).
   * @param argv Argument vector.
   * @return A `ParseResult<T>` containing the parsed struct or an error.
   */
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
  auto parse(int argc, char* argv[]) -> ParseResult<T> {
    std::vector<std::string_view> args;
    args.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) {
      args.emplace_back(argv[i]);
    }
    if (argc > 0) {
      set_program_name(argv[0]);
    }
    return this->parse(std::span<const std::string_view>(args), 1);
  }

  /**
   * @brief Parses `argc`/`argv` starting from a pre-initialised argument struct.
   *
   * Useful when fields of `T` need to be configured before parsing (e.g.
   * `BoundOption` pointer targets or `on_parse` callbacks set on the struct).
   *
   * @param initial The pre-initialised argument struct.
   * @param argc    Argument count (includes program name at index 0).
   * @param argv    Argument vector.
   * @return A `ParseResult<T>` containing the parsed struct or an error.
   */
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
  auto parse(T initial, int argc, char* argv[]) -> ParseResult<T> {
    std::vector<std::string_view> args;
    args.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) {
      args.emplace_back(argv[i]);
    }
    if (argc > 0) {
      set_program_name(argv[0]);
    }
    return this->parse(std::move(initial),
                       std::span<const std::string_view>(args), 1);
  }

  /**
   * @brief Parses a span of string views, skipping `first_index` elements.
   *
   * @param args        The full argument span (may include program name at index
   * 0).
   * @param first_index Index of the first argument to parse (default: 0).
   * @return A `ParseResult<T>` containing the parsed struct or an error.
   */
  auto parse(std::span<const std::string_view> args, std::size_t first_index = 0)
      -> ParseResult<T> {
    ParseResult<T> result;
    parse_body(result, args, first_index);
    return result;
  }

  /**
   * @brief Parses a span with an explicit program name for help output.
   *
   * @param args         The full argument span.
   * @param program_name The name to display in usage/help text.
   * @param first_index  Index of the first argument to parse (default: 0).
   * @return A `ParseResult<T>` containing the parsed struct or an error.
   */
  auto parse(std::span<const std::string_view> args,
             std::string_view program_name, std::size_t first_index = 0)
      -> ParseResult<T> {
    set_program_name(std::string(program_name));
    ParseResult<T> result;
    parse_body(result, args, first_index);
    return result;
  }

  /**
   * @brief Parses a span starting from a pre-initialised argument struct.
   *
   * Useful with `BoundOption` / `on_parse` so that pointer targets and
   * callbacks set before this call are preserved.
   *
   * @param initial     The pre-initialised argument struct.
   * @param args        The full argument span.
   * @param first_index Index of the first argument to parse (default: 0).
   * @return A `ParseResult<T>` containing the parsed struct or an error.
   */
  auto parse(T initial, std::span<const std::string_view> args,
             std::size_t first_index = 0) -> ParseResult<T> {
    ParseResult<T> result;
    result.value = std::move(initial);
    parse_body(result, args, first_index);
    return result;
  }

  /**
   * @brief Parses a span with a pre-initialised struct and explicit program
   * name.
   *
   * @param initial      The pre-initialised argument struct.
   * @param args         The full argument span.
   * @param program_name The name to display in usage/help text.
   * @param first_index  Index of the first argument to parse (default: 0).
   * @return A `ParseResult<T>` containing the parsed struct or an error.
   */
  auto parse(T initial, std::span<const std::string_view> args,
             std::string_view program_name, std::size_t first_index = 0)
      -> ParseResult<T> {
    set_program_name(program_name);
    ParseResult<T> result;
    result.value = std::move(initial);
    parse_body(result, args, first_index);
    return result;
  }

  /**
   * @brief Get the program name used in help output.
   * @return The program name string.
   */
  [[nodiscard]] auto program_name() const -> const std::string& {
    return program_name_;
  }

  /**
   * @brief Set the program name used in help output.
   * @param name The program name string to set.
   */
  auto set_program_name(std::string name) -> void {
    program_name_ = std::move(name);
  }

 private:
  TokenizerConfig cfg_;
  std::string program_name_ = "program";

  auto parse_body(ParseResult<T>& result, std::span<const std::string_view> args,
                  std::size_t first_index) -> void {
    auto lookup = get_option_spec_map(result.value);

    auto tokenized = tokenize(
        args.subspan(std::min(first_index, args.size())),
        [&](std::string_view opt) -> auto { return lookup.option_nargs(opt); },
        [&](std::string_view name) -> auto { return lookup.is_command(name); },
        cfg_);
    if (tokenized.has_error()) {
      if (tokenized.error.has_position()) {
        tokenized.error.position += static_cast<int>(first_index);
      }
      result.error = std::move(tokenized.error);
      return;
    }

    const auto prefix_view = [&](std::uint8_t p) -> std::string_view {
      if (p == no_prefix()) return {};
      if (is_short_prefix(p)) return cfg_.short_option_prefix();
      return cfg_.option_prefixes()[p & kPrefixIndexMask];
    };

    const auto& tokens = tokenized.tokens;
    std::size_t pos_idx = 0, pos_cnt = 0;

    for (std::size_t i = 0; i < tokens.size();) {
      const Token& tok = tokens[i];

      if (tok.type == TokenType::option) {
        std::size_t j = i + 1;
        while (j < tokens.size() && tokens[j].type == TokenType::value) ++j;
        auto err =
            dispatch_option(result.value, prefix_view(tok.prefix), tok.text,
                            std::span(tokens).subspan(i + 1, j - i - 1),
                            first_index, tok.position);
        if (err.has_error()) {
          result.error = finalize_special_error(std::move(err.error));
          return;
        }
        i = j;

      } else if (tok.type == TokenType::positional) {
        auto err = dispatch_positional(result.value, pos_idx, pos_cnt, tok.text,
                                       tok.position + first_index);
        if (err.has_error()) {
          result.error = finalize_special_error(std::move(err.error));
          return;
        }
        ++i;

      } else if (tok.type == TokenType::command) {
        auto err = dispatch_command(result.value, tok.text, args,
                                    tok.position + first_index + 1);
        if (err.has_error()) {
          result.error = finalize_special_error(std::move(err.error));
          return;
        }
        break;

      } else {
        ++i;
      }
    }

    // Fire on_parse callbacks for positionals once, after all tokens are
    // consumed.
    for_each_field(result.value, [](auto& f) -> auto {
      using F = std::remove_cvref_t<decltype(f)>;
      if constexpr (std::derived_from<F, PositionalTag>) {
        if (f.provided()) f.on_parse();
      }
    });

    if (auto err = apply_env_fallback(result.value); err.has_error()) {
      result.error = std::move(err.error);
      return;
    }

    if (auto err = validate_required(result.value); err.has_error()) {
      result.error = std::move(err.error);
      return;
    }

    if (auto err = validate_relations(result.value); err.has_error()) {
      result.error = std::move(err.error);
      return;
    }

    if constexpr (requires(T& t) {
                    { t.constraints() } -> std::same_as<ConstraintResult>;
                  }) {
      if (auto cr = result.value.constraints(); !cr.ok()) {
        result.error = std::move(*cr.error);
      }
    }
  }

 public:
  /**
   * @brief Generates a help string for the argument specification.
   *
   * @param color_mode Controls ANSI color output. Default: `ColorMode::detect`.
   * @return The formatted help string.
   */
  auto format_help(ColorMode color_mode = ColorMode::detect,
                   HelpPalette palette = default_help_palette) -> std::string {
    return cli::format_help<T>(scratch_, program_name_, color_mode, false,
                               palette);
  }

  /**
   * @brief Generates a help string including recursive subcommand help.
   *
   * @return The formatted help string with all subcommand sections appended.
   */
  auto format_help(RecurseHelpTag) -> std::string {
    return format_help(ColorMode::detect, recurse_help);
  }

  auto format_help(HelpPalette palette) -> std::string {
    return format_help(ColorMode::detect, palette);
  }

  /**
   * @brief Generates a help string with explicit color mode and recursive
   * subcommand help.
   *
   * @param color_mode Controls ANSI color output.
   * @return The formatted help string with all subcommand sections appended.
   */
  auto format_help(ColorMode color_mode, RecurseHelpTag) -> std::string {
    return cli::format_help<T>(scratch_, program_name_, color_mode, true);
  }

  auto format_help(ColorMode color_mode, HelpPalette palette, RecurseHelpTag)
      -> std::string {
    return cli::format_help<T>(scratch_, program_name_, color_mode, true,
                               palette);
  }

 private:
  T scratch_{};

  // Iterate every field of val, calling fn(field) for each.
  template <class Fn>
  static constexpr void for_each_field(T& val, Fn fn) {
    auto&& tup = as_tuple(val);
    [&]<std::size_t... I>(std::index_sequence<I...>)
        -> auto {
      (fn(std::get<I>(tup)), ...);
    }(std::make_index_sequence<
            std::tuple_size_v<std::remove_reference_t<decltype(tup)>>>{});
  }

  template <class Fn>
  static constexpr void for_each_field(const T& val, Fn fn) {
    auto&& tup = as_tuple(val);
    [&]<std::size_t... I>(std::index_sequence<I...>)
        -> auto {
      (fn(std::get<I>(tup)), ...);
    }(std::make_index_sequence<
            std::tuple_size_v<std::remove_reference_t<decltype(tup)>>>{});
  }

  // Build the option spec-map and command-name set from the field types of val.
  struct Lookup {
    const Parser* self;
    const T* val;

    [[nodiscard]] auto option_nargs(std::string_view opt) const
        -> std::optional<Nargs> {
      std::optional<Nargs> found;
      self->for_each_field(*val, [&](const auto& f) -> auto {
        using F = std::remove_cvref_t<decltype(f)>;
        if constexpr (std::derived_from<F, OptionTag>) {
          for (const auto& p : self->cfg_.option_prefixes()) {
            if (opt.starts_with(p) && opt.substr(p.size()) == F::name.view()) {
              found = F::nargs;
            }
          }
          if constexpr (F::short_name != '\0') {
            const auto& sp = self->cfg_.short_option_prefix();
            if (opt.starts_with(sp) && opt.size() == sp.size() + 1 &&
                opt.back() == F::short_name) {
              found = F::nargs;
            }
          }
        }
      });
      return found;
    }

    [[nodiscard]] auto is_command(std::string_view name) const -> bool {
      bool found = false;
      self->for_each_field(*val, [&](const auto& f) -> auto {
        using F = std::remove_cvref_t<decltype(f)>;
        if constexpr (std::derived_from<F, CommandTag>) {
          if (F::command_name() == name) found = true;
        }
      });
      return found;
    }
  };

  auto get_option_spec_map(const T& val) const -> Lookup {
    return Lookup{.self = this, .val = &val};
  }

  // Find the option field matching (prefix, bare), notify it, and invoke its
  // action on each value token. notify_option_seen() is called once;
  // invoke_action() is called once for flags / missing optional entries, or
  // once per value token otherwise.
  auto dispatch_option(T& val, std::string_view prefix, std::string_view bare,
                       std::span<const Token> vals, std::size_t first_index,
                       std::size_t option_position) -> ActionResult<void> {
    ActionResult<void> res = ActionResult<void>::ok();
    bool found = false;
    for_each_field(val, [&](auto& f) -> auto {
      using F = std::remove_cvref_t<decltype(f)>;
      if constexpr (std::derived_from<F, OptionTag>) {
        if (found) {
          return;
        }
        bool match = false;
        for (const auto& p : cfg_.option_prefixes()) {
          if (p == prefix && F::name.view() == bare) {
            match = true;
            break;
          }
        }
        if (!match && F::short_name != '\0' &&
            cfg_.short_option_prefix() == prefix && bare.size() == 1 &&
            bare[0] == F::short_name) {
          match = true;
        }
        if (!match) {
          return;
        }
        found = true;
        f.notify_option_seen();
        if constexpr (F::entry_is_optional) {
          res = f.invoke_action(std::nullopt, option_position + first_index,
                                vals.size(), 0);
        }
        const std::size_t nargs = vals.size();
        for (std::size_t vi = 0; vi < vals.size(); ++vi) {
          if (res.has_error()) {
            break;
          }
          res = f.invoke_action(vals[vi].text, vals[vi].position + first_index,
                                nargs, vi);
        }
        if (!res.has_error()) f.on_parse();
      }
    });
    return res;
  }

  // Fill the pos_idx-th positional field with text.  Advances pos_idx/pos_cnt
  // when the current field reaches its nargs.max.
  static auto dispatch_positional(T& val, std::size_t& pos_idx,
                                  std::size_t& pos_cnt, std::string_view text,
                                  std::size_t position) -> ActionResult<void> {
    ActionResult<void> res = ActionResult<void>::ok();
    bool invoked = false;
    std::size_t cur = 0;
    for_each_field(val, [&](auto& f) -> auto {
      using F = std::remove_cvref_t<decltype(f)>;
      if constexpr (std::derived_from<F, PositionalTag>) {
        if (cur++ != pos_idx || invoked) return;
        invoked = true;
        res = f.invoke_action(text, position);
        if (res.has_value()) {
          ++pos_cnt;
          if (F::nargs.max != -1 &&
              pos_cnt >= static_cast<std::size_t>(F::nargs.max)) {
            ++pos_idx;
            pos_cnt = 0;
          }
        }
      }
    });
    if (!invoked)
      return ActionResult<void>::fail(
          ParseError{.code = ErrorCode::unexpected_argument,
                     .kind = ErrorKind::parse,
                     .position = static_cast<int>(position),
                     .subject = std::string(text)});
    return res;
  }

  auto dispatch_command(T& val, std::string_view name,
                        std::span<const std::string_view> args,
                        std::size_t first_index) -> ActionResult<void> {
    ActionResult<void> res = ActionResult<void>::ok();
    bool found = false;
    for_each_field(val, [&](auto& f) -> auto {
      using F = std::remove_cvref_t<decltype(f)>;
      if constexpr (std::derived_from<F, CommandTag>) {
        if (found || F::command_name() != name) {
          return;
        }
        found = true;
        f.mark_provided();
        auto sub_parser = Parser<typename F::argument_type>{};
        sub_parser.set_program_name(program_name_ + " " +
                                    std::string(F::command_name()));
        auto sub = sub_parser.parse(args, first_index);
        if (sub.has_error()) {
          res = ActionResult<void>::fail(std::move(sub.error));
        } else {
          static_cast<typename F::argument_type&>(f) = std::move(sub.value);
          f.mark_provided();
        }
      }
    });
    if (!found) {
      return ActionResult<void>::fail(ParseError{
          .code = ErrorCode::unknown_command,
          .kind = ErrorKind::parse,
          .position = static_cast<int>(first_index - 1),
          .subject = std::string(name),
      });
    }
    return res;
  }

  static auto validate_required(T& val) -> ActionResult<void> {
    ActionResult<void> res = ActionResult<void>::ok();
    for_each_field(val, [&](auto& f) -> auto {
      using F = std::remove_cvref_t<decltype(f)>;
      if constexpr (std::derived_from<F, OptionTag> ||
                    std::derived_from<F, PositionalTag>) {
        if (res.has_error()) {
          return;
        }
        if (f.get_param().presence == Presence::required &&
            !static_cast<bool>(f)) {
          std::string subject;
          if constexpr (std::derived_from<F, OptionTag>) {
            subject = "--" + std::string(F::name.view());
          } else {
            subject = "<positional>";
          }
          res = ActionResult<void>::fail(ParseError{
              .code = ErrorCode::missing_required,
              .kind = ErrorKind::parse,
              .subject = std::move(subject),
          });
        }
      }
    });
    return res;
  }

  static auto relation_name_list(NameList<> names) -> std::string {
    std::string out;
    for (std::size_t i = 0; i < names.size; ++i) {
      if (i != 0) out += ", ";
      out += "--";
      out += names.values[i];
    }
    return out;
  }

  static auto relation_group_exists(std::string_view name) -> bool {
    if constexpr (!detail::HasRelations<T>) {
      return false;
    } else {
      bool found = false;
      std::apply(
          [&](auto... rels) -> auto {
            (
                [&]() -> auto {
                  using R = std::remove_cvref_t<decltype(rels)>;
                  if constexpr (std::same_as<R, GroupRelation>) {
                    if (rels.name == name) found = true;
                  }
                }(),
                ...);
          },
          T::relations.items);
      return found;
    }
  }

  static auto relation_operand_label(std::string_view operand) -> std::string {
    if (relation_group_exists(operand)) {
      return std::string(operand);
    }
    return "--" + std::string(operand);
  }

  static auto relation_operand_present(T& val, std::string_view name) -> bool {
    bool found_field = false;
    bool present = false;
    for_each_field(val, [&](auto& f) -> auto {
      using F = std::remove_cvref_t<decltype(f)>;
      if constexpr (std::derived_from<F, OptionTag> ||
                    std::derived_from<F, CommandTag>) {
        if (!found_field && F::name.view() == name) {
          found_field = true;
          present = f.provided();
        }
      }
    });
    if (found_field) return present;

    if constexpr (detail::HasRelations<T>) {
      bool found_group = false;
      bool group_present = false;
      std::apply(
          [&](auto... rels) -> auto {
            (
                [&]() -> auto {
                  using R = std::remove_cvref_t<decltype(rels)>;
                  if constexpr (std::same_as<R, GroupRelation>) {
                    if (!found_group && rels.name == name) {
                      found_group = true;
                      group_present = true;
                      for (auto member : rels.members) {
                        group_present = group_present &&
                                        relation_operand_present(val, member);
                      }
                    }
                  }
                }(),
                ...);
          },
          T::relations.items);
      return group_present;
    }
    return false;
  }

  static auto validate_relations(T& val) -> ActionResult<void> {
    if constexpr (!detail::HasRelations<T>) {
      return ActionResult<void>::ok();
    } else {
      ActionResult<void> res = ActionResult<void>::ok();
      std::apply(
          [&](auto... rels) -> auto {
            (
                [&]() -> auto {
                  if (res.has_error()) return;
                  using R = std::remove_cvref_t<decltype(rels)>;
                  if constexpr (std::same_as<R, GroupRelation>) {
                    std::size_t count = 0;
                    for (auto member : rels.members) {
                      if (relation_operand_present(val, member)) ++count;
                    }
                    if (count != 0 && count != rels.members.size) {
                      res = ActionResult<void>::fail(ParseError{
                          .code = ErrorCode::dependency_missing,
                          .kind = ErrorKind::validation,
                          .subject = std::string(rels.name),
                          .detail = "arguments must be provided together: " +
                                    relation_name_list(rels.members),
                      });
                    }
                  } else if constexpr (std::same_as<R, ConflictsRelation>) {
                    if (relation_operand_present(val, rels.left) &&
                        relation_operand_present(val, rels.right)) {
                      res = ActionResult<void>::fail(ParseError{
                          .code = ErrorCode::mutually_exclusive,
                          .kind = ErrorKind::validation,
                          .subject = relation_operand_label(rels.left) + ", " +
                                     relation_operand_label(rels.right),
                          .detail =
                              "these arguments cannot be provided together",
                      });
                    }
                  } else if constexpr (std::same_as<R, DependsOnRelation>) {
                    if (relation_operand_present(val, rels.source) &&
                        !relation_operand_present(val, rels.target)) {
                      res = ActionResult<void>::fail(ParseError{
                          .code = ErrorCode::dependency_missing,
                          .kind = ErrorKind::validation,
                          .subject = relation_operand_label(rels.source),
                          .detail =
                              "requires " + relation_operand_label(rels.target),
                      });
                    }
                  }
                }(),
                ...);
          },
          T::relations.items);
      return res;
    }
  }

  auto apply_env_fallback(T& val) -> ActionResult<void> {
    ActionResult<void> res = ActionResult<void>::ok();
    for_each_field(val, [&](auto& f) -> auto {
      using F = std::remove_cvref_t<decltype(f)>;
      if constexpr (std::derived_from<F, OptionTag>) {
        if (res.has_error() || f.provided() || f.get_param().env.empty()) return;
#ifdef _WIN32
        char* raw_buf = nullptr;
        std::size_t raw_len = 0;
        _dupenv_s(&raw_buf, &raw_len, std::string(f.get_param().env).c_str());
        std::unique_ptr<char, decltype(&free)> raw_guard(raw_buf, &free);
        const char* raw = raw_buf;
#else
        const char* raw = std::getenv(std::string(f.get_param().env).c_str());
#endif
        if (!raw) return;
        if constexpr (F::nargs.min == 0 && F::nargs.max == 0) {
          res = f.invoke_action(std::nullopt, 0, 0, 0, false);
        } else {
          res = f.invoke_action(std::string_view(raw), 0, 1, 0, false);
        }
        if (!res.has_error()) f.on_parse();
      }
    });
    return res;
  }

  auto finalize_special_error(ParseError error) -> ParseError {
    if (error.code == ErrorCode::help_requested && error.detail.empty()) {
      error.detail = format_help(ColorMode::detect);
    }
    return error;
  }
};

/**
 * @brief Convenience free function that parses `argc`/`argv` into type `T`.
 *
 * Constructs a default `Parser<T>` and delegates to `Parser::parse`.
 *
 * @tparam T The argument specification type.
 * @param argc Argument count (includes program name at index 0).
 * @param argv Argument vector.
 * @return A `ParseResult<T>` containing the parsed struct or an error.
 */
template <ArgumentSpec T>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
auto parse(int argc, char* argv[]) -> ParseResult<T> {
  return Parser<T>{}.parse(argc, argv);
}

/**
 * @brief Parses `argc`/`argv` and exits the process on failure.
 *
 * If parsing fails, writes the error message directly to `stderr` (or `stdout`
 * for help/exit-success) using stdio and calls `std::exit` with the appropriate
 * exit code. Include `cli/parser_stdio.hh` or `cli/parser_iostream.hh` when the
 * output destination must be customized.
 *
 * This is the typical one-liner entry point for command-line tools:
 * @code
 *   auto args = cli::parse_or_exit<MyArgs>(argc, argv);
 * @endcode
 *
 * @tparam T   The argument specification type.
 * @param argc Argument count (includes program name at index 0).
 * @param argv Argument vector.
 * @return The parsed argument struct (never returns on failure).
 */
template <ArgumentSpec T>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
auto parse_or_exit(int argc, char* argv[]) -> T {
  auto result = parse<T>(argc, argv);
  if (!result) {
    if (const auto message = result.error.message(); !message.empty()) {
      detail::write_message(result.error.use_stdout() ? stdout : stderr,
                            message);
    }
    std::exit(result.error.exit_code());
  }
  return std::move(result.value);
}

}  // namespace cli
