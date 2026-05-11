#pragma once

#include <concepts>
#include <optional>
#include <string>
#include <string_view>

#include "cli/error.hh"

namespace cli {

/**
 * @brief The result type returned by a `constraints()` post-parse hook.
 *
 * Supports short-circuit composition with `&&` (fail-fast AND) and `||`
 * (first-success OR):
 *
 * @code
 *   auto constraints() -> cli::ConstraintResult {
 *     return cli::constraint::at_most_one_of(verbose, debug)
 *         && cli::constraint::requires_if(output, format);
 *   }
 * @endcode
 *
 * Factory methods:
 * - `ConstraintResult::pass()` — success.
 * - `ConstraintResult::fail(ParseError)` — failure with a specific error.
 */
struct ConstraintResult {
  std::optional<ParseError> error;

  [[nodiscard]] auto ok() const noexcept -> bool { return !error.has_value(); }
  [[nodiscard]] explicit operator bool() const noexcept { return ok(); }

  static auto pass() -> ConstraintResult { return {}; }
  static auto fail(ParseError e) -> ConstraintResult { return {std::move(e)}; }

  friend auto operator&&(ConstraintResult a, ConstraintResult b)
      -> ConstraintResult {
    return a.ok() ? std::move(b) : std::move(a);
  }

  friend auto operator||(ConstraintResult a, ConstraintResult b)
      -> ConstraintResult {
    return a.ok() ? std::move(a) : std::move(b);
  }
};

namespace detail {

template <class T>
concept NamedProviderArg = requires(const T& t) {
  { t.provided() } -> std::convertible_to<bool>;
  {
    std::remove_cvref_t<T>::name.view()
  } -> std::convertible_to<std::string_view>;
};

template <NamedProviderArg... Args>
auto names_list(const Args&...) -> std::string {
  std::string s;
  bool first = true;
  (..., (s += (first ? std::string{} : std::string{", "}) + "--" +
              std::string(std::remove_cvref_t<Args>::name.view()),
         first = false));
  return s;
}

}  // namespace detail

/**
 * @brief Post-parse constraint helpers.
 *
 * Each function checks a relationship between options and returns a
 * `ConstraintResult`.  Combine results with `&&` or `||`:
 *
 * @code
 *   auto constraints() -> cli::ConstraintResult {
 *     return cli::constraint::one_of(foo, bar)
 *         && cli::constraint::requires_if(output, format);
 *   }
 * @endcode
 */
namespace constraint {

/**
 * @brief Exactly one of the listed options must be provided.
 *
 * Returns `validation_failed` / `mutually_exclusive` with a message
 * listing all option names when the count is 0 or ≥ 2.
 */
template <detail::NamedProviderArg... Args>
auto one_of(const Args&... args) -> ConstraintResult {
  int count = (0 + ... + (args.provided() ? 1 : 0));
  if (count == 1) return ConstraintResult::pass();
  auto subject = detail::names_list(args...);
  if (count == 0) {
    return ConstraintResult::fail(ParseError{
        .code = ErrorCode::missing_required,
        .kind = ErrorKind::validation,
        .subject = subject,
        .detail = "exactly one of these options must be provided",
    });
  }
  return ConstraintResult::fail(ParseError{
      .code = ErrorCode::mutually_exclusive,
      .kind = ErrorKind::validation,
      .subject = subject,
      .detail = "exactly one of these options must be provided, but multiple "
                "were given",
  });
}

/**
 * @brief At most one of the listed options may be provided.
 *
 * Returns `mutually_exclusive` if two or more are present simultaneously.
 */
template <detail::NamedProviderArg... Args>
auto at_most_one_of(const Args&... args) -> ConstraintResult {
  int count = (0 + ... + (args.provided() ? 1 : 0));
  if (count <= 1) return ConstraintResult::pass();
  return ConstraintResult::fail(ParseError{
      .code = ErrorCode::mutually_exclusive,
      .kind = ErrorKind::validation,
      .subject = detail::names_list(args...),
      .detail = "at most one of these options may be provided",
  });
}

/**
 * @brief All of the listed options must be provided.
 *
 * Returns `missing_required` listing all names if any are absent.
 */
template <detail::NamedProviderArg... Args>
auto all_of(const Args&... args) -> ConstraintResult {
  bool all = (... && args.provided());
  if (all) return ConstraintResult::pass();
  return ConstraintResult::fail(ParseError{
      .code = ErrorCode::missing_required,
      .kind = ErrorKind::validation,
      .subject = detail::names_list(args...),
      .detail = "all of these options must be provided",
  });
}

/**
 * @brief Either all of the listed options must be provided, or none.
 *
 * Returns `dependency_missing` when some but not all are present.
 */
template <detail::NamedProviderArg... Args>
auto all_or_none(const Args&... args) -> ConstraintResult {
  int count = (0 + ... + (args.provided() ? 1 : 0));
  constexpr int total = static_cast<int>(sizeof...(Args));
  if (count == 0 || count == total) return ConstraintResult::pass();
  return ConstraintResult::fail(ParseError{
      .code = ErrorCode::dependency_missing,
      .kind = ErrorKind::validation,
      .subject = detail::names_list(args...),
      .detail = "either all of these options must be provided, or none",
  });
}

/**
 * @brief If `condition` is provided, `required` must also be provided.
 *
 * Returns `dependency_missing` naming the missing option when the
 * condition holds but the dependency is absent.
 */
template <detail::NamedProviderArg Cond, detail::NamedProviderArg Req>
auto requires_if(const Cond& condition, const Req& required)
    -> ConstraintResult {
  if (!condition.provided() || required.provided())
    return ConstraintResult::pass();
  return ConstraintResult::fail(ParseError{
      .code = ErrorCode::dependency_missing,
      .kind = ErrorKind::validation,
      .subject = "--" + std::string(std::remove_cvref_t<Req>::name.view()),
      .detail = "required when --" +
                std::string(std::remove_cvref_t<Cond>::name.view()) +
                " is provided",
  });
}

}  // namespace constraint

}  // namespace cli
