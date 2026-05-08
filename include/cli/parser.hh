#pragma once

// #include <expected>
#include <cstdlib>
#include <format>
#include <iostream>
#include <span>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "cli/argument.hh"
#include "cli/error.hh"
#include "cli/help.hh"

namespace cli {

#ifdef _WIN32
namespace detail {
/**
 * @brief Converts a wide-character string to UTF-8.
 *
 * Uses `WideCharToMultiByte` with code page `CP_UTF8`. Returns an empty string
 * for a null pointer or a zero-length conversion result.
 *
 * @param wide Null-terminated wide-character string, or `nullptr`.
 * @return The UTF-8 encoded equivalent, or `""` on failure.
 */
inline auto wide_to_utf8(const wchar_t* wide) -> std::string {
  if (!wide) return {};
  int len =
      WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
  if (len <= 1) return {};
  std::string result(static_cast<std::size_t>(len) - 1, '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), len, nullptr,
                      nullptr);
  return result;
}
}  // namespace detail
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
enum class TokenType {
  option,
  value,
  positional,
  command,
};

/**
 * @brief A single token produced by `tokenize()`.
 */
struct Token {
  TokenType type;  ///< Classification of this token.
  std::string_view
      text;  ///< The token text (a view into the original `args` span).
  std::string_view matched_prefix;  ///< The option prefix that was matched (e.g.
                                    ///< `"--"`); empty for non-option tokens.
  std::size_t position;  ///< Zero-based index into the original `args` span.
};

/**
 * @brief The result returned by `tokenize()`.
 *
 * On success, `tokens` contains the ordered token sequence and `error` is in
 * the default (no-error) state. On failure, `tokens` may be partial and
 * `error` describes the first problem encountered.
 */
struct TokenizeResult {
  std::vector<Token> tokens;
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
  std::vector<std::string> option_prefixes = {
      "--"};  ///< Prefixes that introduce long options.
  std::string short_option_prefix =
      "-";  ///< Prefix for short (single-character) options.
  std::string end_of_options_separator =
      "--";  ///< Token that ends option processing; subsequent tokens become
             ///< positionals.
  char inline_value_separator =
      '=';  ///< Character separating option name from inline value (e.g. `=` for
            ///< `--opt=val`).
};

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
inline auto tokenize(std::span<const std::string_view> args,
                     const std::unordered_map<std::string, Nargs>& spec_map,
                     const std::unordered_set<std::string>& command_names = {},
                     const TokenizerConfig& cfg = {}) -> TokenizeResult {
  TokenizeResult result;

  // Returns the matched prefix (as a view into cfg.option_prefixes) or empty.
  const auto find_prefix = [&](std::string_view tok) -> std::string_view {
    for (const auto& p : cfg.option_prefixes) {
      if (tok.size() > p.size() && tok.starts_with(p)) return p;
    }
    if (tok.size() > cfg.short_option_prefix.size() &&
        tok.starts_with(cfg.short_option_prefix)) {
      return cfg.short_option_prefix;
    }
    return {};
  };

  const auto push = [&](TokenType type, std::string_view text, std::size_t pos,
                        std::string_view prefix = {}) -> void {
    result.tokens.push_back(
        {.type = type, .text = text, .matched_prefix = prefix, .position = pos});
  };

  bool post_separator = false;
  std::size_t i = 0;

  while (i < args.size()) {
    const std::string_view tok = args[i];

    // ── option parsing terminator ─────────────────────────────────────────
    if (!post_separator && tok == cfg.end_of_options_separator) {
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
    if (prefix.empty() && command_names.contains(std::string(tok))) {
      push(TokenType::command, tok, i);
      break;  // args[i+1:] are the sub-command's responsibility
    }

    // ── option-looking token ──────────────────────────────────────────────
    if (!prefix.empty()) {
      const auto sep = cfg.inline_value_separator;
      const auto eq_pos = tok.find(sep);
      const bool has_inline_val = (eq_pos != std::string_view::npos);
      const std::string_view opt_sv =
          has_inline_val ? tok.substr(0, eq_pos) : tok;
      const std::string opt_name{opt_sv};

      const auto spec_it = spec_map.find(opt_name);
      if (spec_it == spec_map.end()) {
        return result.fail(ErrorCode::unknown_option, i, opt_name);
      }

      const Nargs& nargs = spec_it->second;
      const bool is_flag = (nargs.min == 0 && nargs.max == 0);

      // Use opt_sv (view into args) for matched_prefix so it stays valid
      // after cfg is destroyed — do not point into cfg.option_prefixes.
      push(TokenType::option, opt_sv, i, opt_sv.substr(0, prefix.size()));

      if (has_inline_val) {
        if (is_flag) {
          return result.fail(ErrorCode::invalid_value, i, opt_name,
                             "flag does not accept a value");
        }
        if (nargs.min > 1) {
          return result.fail(
              ErrorCode::missing_value, i, opt_name,
              std::format("option requires at least {} value(s), but inline "
                          "separator provides only 1",
                          nargs.min));
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

        if (next == cfg.end_of_options_separator) break;
        if (command_names.contains(std::string(next))) break;
        if (nargs.max != -1 && count >= nargs.max) break;

        if (!find_prefix(next).empty()) {
          const auto next_sep = next.find(sep);
          const std::string next_opt{next_sep != std::string_view::npos
                                         ? next.substr(0, next_sep)
                                         : next};
          if (spec_map.contains(next_opt)) break;

          // Unknown option-looking: consume only as first value when min > 0
          if (count >= 1 || nargs.min == 0) break;
        }

        push(TokenType::value, args[i], i);
        ++i;
        ++count;
      }

      if (count < nargs.min) {
        return result.fail(
            ErrorCode::missing_value, i, opt_name,
            std::format("option requires at least {} value(s), but {} provided",
                        nargs.min, count));
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
 * On Windows, `wchar_t*` overloads are also provided; they convert arguments to
 * UTF-8 via `detail::wide_to_utf8` before processing.
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
      program_name_ = argv[0];
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
      program_name_ = argv[0];
    }
    return this->parse(std::move(initial),
                       std::span<const std::string_view>(args), 1);
  }

#ifdef _WIN32
  auto parse(int argc, wchar_t* argv[]) -> ParseResult<T> {
    std::vector<std::string> owned;
    owned.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) {
      owned.emplace_back(detail::wide_to_utf8(argv[i]));
    }
    std::vector<std::string_view> args;
    args.reserve(owned.size());
    for (const auto& s : owned) {
      args.emplace_back(s);
    }
    if (argc > 0) {
      program_name_ = owned[0];
    }
    return this->parse(std::span<const std::string_view>(args), 1);
  }

  auto parse(T initial, int argc, wchar_t* argv[]) -> ParseResult<T> {
    std::vector<std::string> owned;
    owned.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) {
      owned.emplace_back(detail::wide_to_utf8(argv[i]));
    }
    std::vector<std::string_view> args;
    args.reserve(owned.size());
    for (const auto& s : owned) {
      args.emplace_back(s);
    }
    if (argc > 0) {
      program_name_ = owned[0];
    }
    return this->parse(std::move(initial),
                       std::span<const std::string_view>(args), 1);
  }
#endif

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
    program_name_ = std::string(program_name);
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
    program_name_ = std::string(program_name);
    ParseResult<T> result;
    result.value = std::move(initial);
    parse_body(result, args, first_index);
    return result;
  }

 private:
  TokenizerConfig cfg_;
  std::string program_name_ = "program";

  auto parse_body(ParseResult<T>& result, std::span<const std::string_view> args,
                  std::size_t first_index) -> void {
    auto [spec_map, command_names] = get_option_spec_map(result.value);
    auto tokenized = tokenize(args.subspan(std::min(first_index, args.size())),
                              spec_map, command_names, cfg_);
    if (tokenized.has_error()) {
      if (tokenized.error.has_position()) {
        tokenized.error.position += static_cast<int>(first_index);
      }
      result.error = std::move(tokenized.error);
      return;
    }

    const auto& tokens = tokenized.tokens;
    std::size_t pos_idx = 0, pos_cnt = 0;

    for (std::size_t i = 0; i < tokens.size();) {
      const Token& tok = tokens[i];

      if (tok.type == TokenType::option) {
        std::size_t j = i + 1;
        while (j < tokens.size() && tokens[j].type == TokenType::value) ++j;
        auto err = dispatch_option(result.value, tok.matched_prefix,
                                   tok.text.substr(tok.matched_prefix.size()),
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
        if (f.provided()) f.fire_on_parse();
      }
    });

    if (auto err = validate_required(result.value); err.has_error()) {
      result.error = std::move(err.error);
    }
  }

 public:
  /**
   * @brief Generates a help string for the argument specification.
   *
   * @param color_mode Controls ANSI color output. Default: `ColorMode::auto_`.
   * @return The formatted help string.
   */
  auto format_help(ColorMode color_mode = ColorMode::auto_) -> std::string {
    return cli::format_help<T>(scratch_, program_name_, color_mode, false);
  }

  /**
   * @brief Generates a help string including recursive subcommand help.
   *
   * @return The formatted help string with all subcommand sections appended.
   */
  auto format_help(RecurseHelpTag) -> std::string {
    return format_help(ColorMode::auto_, recurse_help);
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

 private:
  T scratch_{};

  // Iterate every field of val, calling fn(field) for each.
  template <class Fn>
  static auto for_each_field(T& val, Fn fn) -> void {
    std::apply([&fn](auto&... f) -> auto { (..., fn(f)); }, as_tuple(val));
  }
  template <class Fn>
  static auto for_each_field(const T& val, Fn fn) -> void {
    std::apply([&fn](const auto&... f) -> auto { (..., fn(f)); }, as_tuple(val));
  }

  // Build the option spec-map and command-name set from the field types of val.
  auto get_option_spec_map(const T& val)
      -> std::pair<std::unordered_map<std::string, Nargs>,
                   std::unordered_set<std::string>> {
    std::unordered_map<std::string, Nargs> spec_map;
    std::unordered_set<std::string> command_names;
    for_each_field(val, [&](const auto& f) -> auto {
      using F = std::remove_cvref_t<decltype(f)>;
      if constexpr (std::derived_from<F, OptionTag>) {
        for (const auto& p : cfg_.option_prefixes)
          spec_map.emplace(p + std::string(F::name.view()), F::nargs);
        if constexpr (F::short_name != '\0')
          spec_map.emplace(
              cfg_.short_option_prefix + std::string(1, F::short_name),
              F::nargs);
      }
      if constexpr (std::derived_from<F, CommandTag>)
        command_names.emplace(std::string(F::command_name()));
    });
    return {spec_map, command_names};
  }

  // Find the option field matching (prefix, bare), notify it, and invoke its
  // action on each value token.  notify_option_seen() is called once;
  // invoke_action() (or invoke_flag() for nargs={0,0}) once per value token.
  auto dispatch_option(T& val, std::string_view prefix, std::string_view bare,
                       std::span<const Token> vals, std::size_t first_index,
                       std::size_t option_position) -> ActionResult<void> {
    ActionResult<void> res = ActionResult<void>::ok();
    bool found = false;
    for_each_field(val, [&](auto& f) -> auto {
      using F = std::remove_cvref_t<decltype(f)>;
      if constexpr (std::derived_from<F, OptionTag>) {
        if (found) return;
        bool match = false;
        for (const auto& p : cfg_.option_prefixes)
          if (p == prefix && F::name.view() == bare) {
            match = true;
            break;
          }
        if (!match && F::short_name != '\0' &&
            cfg_.short_option_prefix == prefix && bare.size() == 1 &&
            bare[0] == F::short_name)
          match = true;
        if (!match) return;
        found = true;
        f.notify_option_seen();
        if constexpr (F::nargs.min == 0 && F::nargs.max == 0) {
          res = f.invoke_flag(option_position + first_index);
        } else {
          for (const auto& vt : vals) {
            if (res.has_error()) break;
            res = f.invoke_action(vt.text, vt.position + first_index);
          }
        }
        if (!res.has_error()) f.fire_on_parse();
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
        sub_parser.program_name_ =
            std::format("{} {}", program_name_, F::command_name());
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
        if (f.presence == Presence::required && !f.provided()) {
          std::string subject;
          if constexpr (std::derived_from<F, OptionTag>) {
            subject = std::string(F::name.view());
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

  auto finalize_special_error(ParseError error) -> ParseError {
    if (error.code == ErrorCode::help_requested && error.detail.empty()) {
      error.detail = format_help(ColorMode::never);
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
 * If parsing fails, writes the error message to `err` (or `out` for
 * help/exit-success) and calls `std::exit` with the appropriate exit code. On
 * success, returns the fully populated argument struct by value.
 *
 * This is the typical one-liner entry point for command-line tools:
 * @code
 *   auto args = cli::parse_or_exit<MyArgs>(argc, argv);
 * @endcode
 *
 * @tparam T   The argument specification type.
 * @param argc Argument count (includes program name at index 0).
 * @param argv Argument vector.
 * @param out  Output stream for help/success messages. Default: `std::cout`.
 * @param err  Output stream for error messages. Default: `std::cerr`.
 * @return The parsed argument struct (never returns on failure).
 */
template <ArgumentSpec T>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
auto parse_or_exit(int argc, char* argv[], std::ostream& out = std::cout,
                   std::ostream& err = std::cerr) -> T {
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

#ifdef _WIN32
template <ArgumentSpec T>
auto parse(int argc, wchar_t* argv[]) -> ParseResult<T> {
  return Parser<T>{}.parse(argc, argv);
}

template <ArgumentSpec T>
auto parse_or_exit(int argc, wchar_t* argv[], std::ostream& out = std::cout,
                   std::ostream& err = std::cerr) -> T {
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
#endif

};  // namespace cli
