#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <filesystem>
#include <format>
#include <functional>
#include <map>
#include <optional>
#include <ranges>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "cli/error.hh"
#include "cli/string_literal.hh"

namespace cli {

/**
 * @brief The result type produced and consumed at each step of an action
 * pipeline.
 *
 * An `ActionResult<T>` is a 3-state type: ok (carrying a value of type `T`),
 * error (carrying a `ParseError`), or ignore (silent skip). The
 * `operator bool()`, `have_value()`, `has_value()`, `has_error()`, and
 * `is_ignored()` methods make it easy to check the state inline.
 *
 * Factory methods:
 * - `ActionResult<T>::ok(value)` — create a success result.
 * - `ActionResult<T>::fail(error)` — create a failure result.
 * - `ActionResult<T>::ignore()` — create an ignore result.
 *
 * @tparam T The value type held on success.
 */
template <class T>
struct ActionResult {
  using value_type = T;

  enum class State : std::uint8_t { ok, error, ignore };

  State state{State::ok};
  ParseError error{};
  T value{};

  /** @brief Returns `true` when the result represents success. */
  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return state == State::ok;
  }

  /** @brief Returns `true` when the result represents success. */
  [[nodiscard]] constexpr auto have_value() const noexcept -> bool {
    return state == State::ok;
  }

  /** @brief Returns `true` when the result represents success. */
  [[nodiscard]] constexpr auto has_value() const noexcept -> bool {
    return have_value();
  }

  /** @brief Returns `true` when the result represents failure. */
  [[nodiscard]] constexpr auto has_error() const noexcept -> bool {
    return state == State::error;
  }

  /** @brief Returns `true` when the result represents an ignored step. */
  [[nodiscard]] constexpr auto is_ignored() const noexcept -> bool {
    return state == State::ignore;
  }

  /**
   * @brief Creates a success result holding `value`.
   *
   * @tparam U Type of the value (deduced).
   * @param v The value to store.
   * @return A successful `ActionResult<T>`.
   */
  template <class U>
  [[nodiscard]] static constexpr auto ok(U&& v) -> ActionResult {
    return {.state = State::ok, .value = std::forward<U>(v)};
  }

  /**
   * @brief Creates a failure result carrying `error`.
   *
   * @param e The error to store.
   * @return A failed `ActionResult<T>`.
   */
  [[nodiscard]] static constexpr auto fail(ParseError e) -> ActionResult {
    return {.state = State::error, .error = std::move(e)};
  }

  /**
   * @brief Creates an ignore result.
   *
   * @return An ignored `ActionResult<T>`.
   */
  [[nodiscard]] static constexpr auto ignore() -> ActionResult {
    return {.state = State::ignore};
  }
};

/**
 * @brief Specialisation of `ActionResult` for terminal (pack) actions that
 * produce no value.
 *
 * Pack actions such as `SetTrue` or `Push` write their result directly into the
 * argument storage referenced by `ActionCtx::arg`; they do not pass a value
 * downstream, so `ActionResult<void>` carries only an error, success, or ignore
 * state.
 */
template <>
struct ActionResult<void> {
  using value_type = void;

  enum class State : std::uint8_t { ok, error, ignore };

  State state{State::ok};
  ParseError error{};

  /** @brief Returns `true` when the result represents success. */
  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return state == State::ok;
  }

  /** @brief Returns `true` when the result represents success. */
  [[nodiscard]] constexpr auto have_value() const noexcept -> bool {
    return state == State::ok;
  }

  /** @brief Returns `true` when the result represents success. */
  [[nodiscard]] constexpr auto has_value() const noexcept -> bool {
    return have_value();
  }

  /** @brief Returns `true` when the result represents failure. */
  [[nodiscard]] constexpr auto has_error() const noexcept -> bool {
    return state == State::error;
  }

  /** @brief Returns `true` when the result represents an ignored step. */
  [[nodiscard]] constexpr auto is_ignored() const noexcept -> bool {
    return state == State::ignore;
  }

  /**
   * @brief Creates a success result.
   *
   * @return A successful `ActionResult<void>`.
   */
  [[nodiscard]] static constexpr auto ok() -> ActionResult<void> {
    return {.state = State::ok};
  }

  /**
   * @brief Creates a failure result carrying `error`.
   *
   * @param e The error to store.
   * @return A failed `ActionResult<void>`.
   */
  [[nodiscard]] static constexpr auto fail(ParseError e) -> ActionResult<void> {
    return {.state = State::error, .error = std::move(e)};
  }

  /**
   * @brief Creates an ignore result.
   *
   * @return An ignored `ActionResult<void>`.
   */
  [[nodiscard]] static constexpr auto ignore() -> ActionResult<void> {
    return {.state = State::ignore};
  }
};

/**
 * @brief Propagates a non-ok ActionResult to a different value type.
 *
 * This preserves error versus ignore state while adapting the held value type.
 *
 * @tparam U The target value type.
 * @tparam T The source value type.
 * @param input The source result to propagate.
 * @return An `ActionResult<U>` carrying the same error or ignore state.
 */
template <class U, class T>
[[nodiscard]] constexpr auto propagate(const ActionResult<T>& input)
    -> ActionResult<U> {
  if (input.has_error()) return ActionResult<U>::fail(input.error);
  return ActionResult<U>::ignore();
}

/**
 * @brief Propagates a non-ok ActionResult<void> to a different value type.
 *
 * @tparam U The target value type.
 * @param input The source result to propagate.
 * @return An `ActionResult<U>` carrying the same error or ignore state.
 */
template <class U>
[[nodiscard]] constexpr auto propagate(const ActionResult<void>& input)
    -> ActionResult<U> {
  if (input.has_error()) return ActionResult<U>::fail(input.error);
  return ActionResult<U>::ignore();
}

/**
 * @brief Context passed to each action function during pipeline execution.
 *
 * Provides access to the current `argv` position, how many times the parent
 * option has been seen, how many values are in this occurrence, the 0-based
 * index of the current value, the total values across all occurrences, and
 * a reference to the argument's storage value.
 *
 * @tparam T The storage type of the argument; `void` for intermediate
 * (non-terminal) actions.
 */
template <class T = void>
struct ActionCtx {
  size_t index{};  ///< Zero-based index into `argv` of the current token.
  size_t
      occurrences{};  ///< Number of times the parent option token has appeared.
  size_t nargs{};     ///< Number of values in this option occurrence.
  size_t value_index{};  ///< 0-based index of current value in this occurrence.
  size_t
      total_values{};  ///< Total values across all occurrences of this option.
  std::reference_wrapper<T>
      arg{};  ///< Reference to the argument's storage value.
};

/**
 * @brief Partial specialisation of `ActionCtx` used for intermediate pipeline
 * steps.
 *
 * Conversion and validation actions do not have access to the final storage;
 * they receive `ActionCtx<void>` which carries only the positional counters.
 * This specialisation also provides a converting constructor from `ActionCtx<T>`
 * so that the parser can downcast when invoking non-terminal actions.
 */
template <>
struct ActionCtx<void> {
  size_t index{};  ///< Zero-based index into `argv` of the current token.
  size_t
      occurrences{};  ///< Number of times the parent option token has appeared.
  size_t nargs{};     ///< Number of values in this option occurrence.
  size_t value_index{};  ///< 0-based index of current value in this occurrence.
  size_t
      total_values{};  ///< Total values across all occurrences of this option.

  /**
   * @brief Constructs from a typed `ActionCtx<T>`, copying the counters.
   *
   * @tparam T The storage type of the source context.
   * @param other The source context to copy counters from.
   */
  template <class T>
  ActionCtx(const ActionCtx<T>& other)
      : index(other.index),
        occurrences(other.occurrences),
        nargs(other.nargs),
        value_index(other.value_index),
        total_values(other.total_values) {}

  ActionCtx() = default;
};

namespace detail {

template <class T>
[[nodiscard]]
auto to_error_subject(const T& value) -> std::string {
  using U = std::remove_cvref_t<T>;

  if constexpr (std::same_as<U, std::string>) {
    return value;
  } else if constexpr (std::same_as<U, std::string_view>) {
    return std::string(value);
  } else if constexpr (std::same_as<U, const char*>) {
    return value == nullptr ? std::string{} : std::string(value);
  } else if constexpr (std::is_same_v<U, bool>) {
    return value ? "true" : "false";
  } else if constexpr (std::integral<U>) {
    return std::to_string(value);
  } else if constexpr (std::same_as<U, std::filesystem::path>) {
    return value.string();
  } else if constexpr (requires { std::format("{}", value); } or
                       std::floating_point<U>) {
    return std::format("{}", value);
  } else {
    return "<value>";
  }
}

[[nodiscard]]
inline auto invalid_value_error(ErrorKind kind, std::size_t index,
                                std::string subject, std::string detail = {})
    -> ParseError {
  return ParseError{
      .code = ErrorCode::invalid_value,
      .kind = kind,
      .position = static_cast<int>(index),
      .subject = std::move(subject),
      .detail = std::move(detail),
  };
}

[[nodiscard]]
inline auto duplicate_argument_error(std::size_t index, std::string subject,
                                     std::string detail = {}) -> ParseError {
  return ParseError{
      .code = ErrorCode::duplicate_argument,
      .kind = ErrorKind::validation,
      .position = static_cast<int>(index),
      .subject = std::move(subject),
      .detail = std::move(detail),
  };
}

[[nodiscard]]
inline auto validation_failed_error(std::size_t index, std::string subject,
                                    std::string detail) -> ParseError {
  return ParseError{
      .code = ErrorCode::validation_failed,
      .kind = ErrorKind::validation,
      .position = static_cast<int>(index),
      .subject = std::move(subject),
      .detail = std::move(detail),
  };
}

[[nodiscard]]
inline auto invalid_choice_error(std::size_t index, std::string subject,
                                 std::string detail = {}) -> ParseError {
  return ParseError{
      .code = ErrorCode::invalid_choice,
      .kind = ErrorKind::conversion,
      .position = static_cast<int>(index),
      .subject = std::move(subject),
      .detail = std::move(detail),
  };
}

[[nodiscard]]
inline auto is_blank(std::string_view text) -> bool {
  return std::ranges::all_of(
      text, [](unsigned char ch) -> bool { return std::isspace(ch) != 0; });
}

template <class T>
using decay_t = std::remove_cvref_t<T>;

template <class T>
struct HelpRequested {
  T value;
};

template <class T>
struct IsHelpRequested : std::false_type {};

template <class T>
struct IsHelpRequested<HelpRequested<T>> : std::true_type {};

template <class T>
concept PairLike = requires {
  typename std::tuple_size<decay_t<T>>::type;
  requires std::tuple_size_v<decay_t<T>> == 2;
};

template <class T>
concept StringLike = std::same_as<decay_t<T>, std::string> ||
                     std::same_as<decay_t<T>, std::string_view>;

}  // namespace detail

/**
 * @brief A compile-time pipeline of action functions applied sequentially to a
 * parsed token.
 *
 * Each element of `Fns` must be a constexpr-constructible callable that:
 * - Exposes `template<class I> static constexpr bool accepts_input` — whether it
 *   can handle input of type `I`.
 * - Exposes `template<class I> using after_type` — the output type when given
 * input `I`.
 * - Exposes `template<class I> using storage_type` — the argument storage type
 *   (non-void only for the last/terminal action in the pipeline).
 *
 * Pipelines are composed with `operator|`:
 * @code
 *   constexpr auto my_action = cli::conversion::integer<int>
 *                            | cli::validation::positive
 *                            | cli::pack::set_once;
 * @endcode
 *
 * At pipeline invocation, `ignore` short-circuits the remaining steps while
 * `error` is passed through so downstream steps can preserve or react to it.
 *
 * @tparam Fns Pack of constexpr action objects forming the pipeline.
 */
template <auto... Fns>
  requires(requires {
    {
      std::remove_cvref_t<decltype(Fns)>::template accepts_input<void>
    } -> std::convertible_to<bool>;
    typename std::remove_cvref_t<decltype(Fns)>::template storage_type<void>;
  } && ...)
struct Action {
  template <class Arg, class Result>
  [[nodiscard]]
  static constexpr auto invoke(ActionCtx<Arg>& ctx, ActionResult<Result> input) {
    return []<auto FnHead, auto... FnTail>(
               ActionCtx<Arg>& ctx,
               ActionResult<Result> input) -> decltype(auto) {
      using Head = std::remove_cvref_t<decltype(FnHead)>;
      using Next = typename Head::template after_type<Result>;

      if (input.is_ignored()) {
        if constexpr (sizeof...(FnTail) == 0) {
          return ActionResult<Next>::ignore();
        } else {
          return Action<FnTail...>::invoke(ctx, ActionResult<Next>::ignore());
        }
      }

      auto next = FnHead(ctx, std::move(input));
      if constexpr (sizeof...(FnTail) == 0) {
        return next;
      } else {
        return Action<FnTail...>::invoke(ctx, std::move(next));
      }
    }.template operator()<Fns...>(ctx, std::move(input));
  }

  template <class Arg, class Result>
  [[nodiscard]]
  static constexpr auto invoke(const ActionCtx<Arg>& ctx,
                               ActionResult<Result> input) {
    return []<auto FnHead, auto... FnTail>(
               const ActionCtx<Arg>& ctx,
               ActionResult<Result> input) -> decltype(auto) {
      using Head = std::remove_cvref_t<decltype(FnHead)>;
      using Next = typename Head::template after_type<Result>;

      if (input.is_ignored()) {
        if constexpr (sizeof...(FnTail) == 0) {
          return ActionResult<Next>::ignore();
        } else {
          return Action<FnTail...>::invoke(ctx, ActionResult<Next>::ignore());
        }
      }

      auto next = FnHead(ctx, std::move(input));
      if constexpr (sizeof...(FnTail) == 0) {
        return next;
      } else {
        return Action<FnTail...>::invoke(ctx, std::move(next));
      }
    }.template operator()<Fns...>(ctx, std::move(input));
  }

  template <class Prev, auto FnHead, auto... FnTail>
  static constexpr auto validate_impl() {
    static_assert(
        sizeof...(FnTail) != 0 ||
            !std::same_as<typename std::remove_cvref_t<
                              decltype(FnHead)>::template storage_type<Prev>,
                          void>,
        "The last action must have a non-void storage type");

    using Head = std::remove_cvref_t<decltype(FnHead)>;
    if constexpr (sizeof...(FnTail) == 0) {
      return std::make_pair(
          Head::template accepts_input<Prev>,
          std::type_identity<typename Head::template storage_type<Prev>>{});
    } else {
      if constexpr (!Head::template accepts_input<Prev>) {
        return std::make_pair(false, std::type_identity<void>{});
      } else {
        using Next = typename Head::template after_type<Prev>;
        return validate_impl<Next, FnTail...>();
      }
    }
  }

 private:
  template <auto First, auto...>
  struct first_fn_type {
    using type = std::remove_cvref_t<decltype(First)>;
  };

 public:
  static constexpr bool entry_is_optional = first_fn_type<
      Fns...>::type::template accepts_input<std::optional<std::string_view>>;

  static constexpr auto validate() -> auto {
    using FirstFn = typename first_fn_type<Fns...>::type;
    if constexpr (FirstFn::template accepts_input<
                      std::optional<std::string_view>>) {
      return validate_impl<std::optional<std::string_view>, Fns...>();
    } else {
      return validate_impl<std::string_view, Fns...>();
    }
  }
};

template <>
struct Action<> {
  static constexpr bool entry_is_optional = false;

  template <class Arg, class Result>
  [[nodiscard]]
  static constexpr auto invoke(ActionCtx<Arg>&, ActionResult<Result> input) {
    return input;
  }

  static constexpr auto validate() -> auto {
    return std::make_pair(true, std::type_identity<std::monostate>{});
  }
};

template <auto Fn, auto... Fns>
constexpr auto operator|(Action<Fns...>, Action<Fn>) {
  return Action<Fns..., Fn>{};
}

/**
 * @brief Deduces `accepts_input` for a non-terminal step from its `operator()`.
 *
 * Evaluates to `true` when `Step::operator()` is callable with
 * `ActionCtx<void>` and `ActionResult<Input>`, i.e. the step accepts a
 * pipeline value of type `Input`.  Use this to avoid writing the
 * `accepts_input` trait by hand for conversion and validation steps:
 *
 * ```cpp
 * struct MyStep {
 *   template <class Input>
 *   static constexpr bool accepts_input =
 *       cli::deduce_accepts_input<MyStep, Input>;
 *   // ...
 * };
 * ```
 *
 * @tparam Step  The action step type.
 * @tparam Input The pipeline value type to test.
 */
template <class Step, class Input>
inline constexpr bool deduce_accepts_input =
    std::invocable<const Step&, ActionCtx<void>, ActionResult<Input>>;

/**
 * @brief Deduces `after_type` for a non-terminal step from its `operator()`.
 *
 * Extracts the value type `T` from the `ActionResult<T>` returned by
 * `Step::operator()(ActionCtx<void>, ActionResult<Input>)`.
 *
 * This alias is **ill-formed** when `deduce_accepts_input<Step, Input>` is
 * `false`, i.e. `operator()` is not callable with `ActionResult<Input>`.
 * This is intentional: `after_type` is only meaningful for compatible inputs,
 * and a compile error at the alias instantiation is clearer than a silent
 * fallback.  Because `validate_impl` always checks `accepts_input` before
 * accessing `after_type`, the ill-formed case is never reached inside a
 * well-formed pipeline.
 *
 * ```cpp
 * struct MyStep {
 *   template <class Input>
 *   using after_type = cli::deduce_after_type<MyStep, Input>;
 *   // ...
 * };
 * ```
 *
 * @tparam Step  The action step type.
 * @tparam Input The pipeline value type.
 */
template <class Step, class Input>
  requires deduce_accepts_input<Step, Input>
using deduce_after_type =
    typename std::invoke_result_t<const Step&, ActionCtx<void>,
                                  ActionResult<Input>>::value_type;

/**
 * @brief Actions that convert a raw `std::string_view` token to a typed value.
 *
 * Each action in this namespace transforms the pipeline value from a string
 * representation into a specific C++ type. They are typically placed first in
 * an action pipeline, before validation and pack actions.
 *
 * Example:
 * @code
 *   constexpr auto my_action = cli::conversion::integer<int> |
 * cli::pack::set_once;
 * @endcode
 */
namespace conversion {

/**
 * @brief Converts a string token to an integral type `T` using
 * `std::from_chars`.
 *
 * Returns `ErrorCode::invalid_value` for non-numeric or partially-consumed
 * input, and `ErrorCode::out_of_range` when the value overflows `T`.
 *
 * @tparam T The target integral type (e.g. `int`, `unsigned long`).
 */
template <std::integral T>
struct Integer {
  template <class Input>
  static constexpr bool accepts_input =
      std::same_as<std::remove_cvref_t<Input>, std::string_view> ||
      std::same_as<std::remove_cvref_t<Input>, std::string>;

  template <class Input>
  using after_type = T;

  template <class Prev>
  using storage_type = void;

  constexpr auto operator()(ActionCtx<void> ctx,
                            ActionResult<std::string_view> input) const
      -> ActionResult<T> {
    if (!input.have_value()) return propagate<T>(input);
    T result{};
    auto r = std::from_chars(input.value.data(),
                             input.value.data() + input.value.size(), result);

    if (r.ec == std::errc::invalid_argument) {
      return ActionResult<T>::fail(detail::invalid_value_error(
          ErrorKind::conversion, ctx.index, std::string(input.value)));
    }
    if (r.ec == std::errc::result_out_of_range) {
      return ActionResult<T>::fail(ParseError{
          .code = ErrorCode::out_of_range,
          .kind = ErrorKind::conversion,
          .position = static_cast<int>(ctx.index),
          .subject = std::string(input.value),
      });
    }
    if (r.ptr != input.value.data() + input.value.size()) {
      return ActionResult<T>::fail(detail::invalid_value_error(
          ErrorKind::conversion, ctx.index, std::string(input.value),
          "unexpected trailing characters"));
    }

    return ActionResult<T>::ok(result);
  }
};

/**
 * @brief Converts a string token to a floating-point type `T` using
 * `std::from_chars`.
 *
 * Returns `ErrorCode::invalid_value` for non-numeric or partially-consumed
 * input, and `ErrorCode::out_of_range` on overflow.
 *
 * @tparam T The target floating-point type (e.g. `float`, `double`).
 */
template <std::floating_point T>
struct Floating {
  template <class Input>
  static constexpr bool accepts_input =
      std::same_as<std::remove_cvref_t<Input>, std::string_view> ||
      std::same_as<std::remove_cvref_t<Input>, std::string>;

  template <class Input>
  using after_type = T;

  template <class Prev>
  using storage_type = void;

  constexpr auto operator()(ActionCtx<void> ctx,
                            ActionResult<std::string_view> input) const
      -> ActionResult<T> {
    if (!input.have_value()) return propagate<T>(input);
    T result{};
    auto r = std::from_chars(input.value.data(),
                             input.value.data() + input.value.size(), result);

    if (r.ec == std::errc::invalid_argument) {
      return ActionResult<T>::fail(detail::invalid_value_error(
          ErrorKind::conversion, ctx.index, std::string(input.value)));
    }
    if (r.ec == std::errc::result_out_of_range) {
      return ActionResult<T>::fail(ParseError{
          .code = ErrorCode::out_of_range,
          .kind = ErrorKind::conversion,
          .position = static_cast<int>(ctx.index),
          .subject = std::string(input.value),
      });
    }
    if (r.ptr != input.value.data() + input.value.size()) {
      return ActionResult<T>::fail(detail::invalid_value_error(
          ErrorKind::conversion, ctx.index, std::string(input.value),
          "unexpected trailing characters"));
    }

    return ActionResult<T>::ok(result);
  }
};

/** @brief Converts a `string_view` token to a `std::string` by copying. */
struct String {
  template <class Input>
  static constexpr bool accepts_input =
      std::same_as<std::remove_cvref_t<Input>, std::string_view> ||
      std::same_as<std::remove_cvref_t<Input>, std::string>;

  template <class Input>
  using after_type = std::string;

  template <class Prev>
  using storage_type = void;

  constexpr auto operator()(ActionCtx<void>,
                            ActionResult<std::string_view> input) const
      -> ActionResult<std::string> {
    if (!input.have_value()) return propagate<std::string>(input);
    return ActionResult<std::string>::ok(std::string(input.value));
  }
};

/**
 * @brief Converts a string token to `bool`.
 *
 * Accepts `"true"` / `"1"` → `true` and `"false"` / `"0"` → `false`.
 * Returns `ErrorCode::invalid_value` for any other input.
 */
struct Bool {
  template <class Input>
  static constexpr bool accepts_input =
      std::same_as<std::remove_cvref_t<Input>, std::string_view> ||
      std::same_as<std::remove_cvref_t<Input>, std::string>;

  template <class Input>
  using after_type = bool;

  template <class Prev>
  using storage_type = void;

  constexpr auto operator()(ActionCtx<void> ctx,
                            ActionResult<std::string_view> input) const
      -> ActionResult<bool> {
    if (!input.have_value()) return propagate<bool>(input);
    const std::string_view val = input.value;
    if (val == "true" || val == "1") {
      return ActionResult<bool>::ok(true);
    }
    if (val == "false" || val == "0") {
      return ActionResult<bool>::ok(false);
    }
    return ActionResult<bool>::fail(detail::invalid_value_error(
        ErrorKind::conversion, ctx.index, std::string(input.value),
        "expected one of: true, false, 1, 0"));
  }
};

/** @brief Converts a string token to `std::filesystem::path` without filesystem
 * validation. */
struct Path {
  template <class Input>
  static constexpr bool accepts_input =
      std::same_as<std::remove_cvref_t<Input>, std::string_view> ||
      std::same_as<std::remove_cvref_t<Input>, std::string>;

  template <class Input>
  using after_type = std::filesystem::path;

  template <class Prev>
  using storage_type = void;

  auto operator()(ActionCtx<void>, ActionResult<std::string_view> input) const
      -> ActionResult<std::filesystem::path> {
    if (!input.have_value()) return propagate<std::filesystem::path>(input);
    return ActionResult<std::filesystem::path>::ok(
        std::filesystem::path{input.value});
  }
};

/**
 * @brief Converts a string token to `std::filesystem::path`, requiring the path
 * to exist and be a regular file.
 *
 * Returns `ErrorCode::invalid_value` if the path does not exist or is not a
 * regular file.
 */
struct ExistingFile {
  template <class Input>
  static constexpr bool accepts_input = Path::template accepts_input<Input>;

  template <class Input>
  using after_type = std::filesystem::path;

  template <class Prev>
  using storage_type = void;

  auto operator()(ActionCtx<void> ctx,
                  ActionResult<std::string_view> input) const
      -> ActionResult<std::filesystem::path> {
    if (!input.have_value()) return propagate<std::filesystem::path>(input);
    auto path = std::filesystem::path{input.value};
    if (!std::filesystem::exists(path) ||
        !std::filesystem::is_regular_file(path)) {
      return ActionResult<std::filesystem::path>::fail(
          detail::invalid_value_error(ErrorKind::conversion, ctx.index,
                                      path.string(),
                                      "expected an existing regular file"));
    }
    return ActionResult<std::filesystem::path>::ok(std::move(path));
  }
};

/**
 * @brief Converts a string token to `std::filesystem::path`, requiring the path
 * to exist and be a directory.
 *
 * Returns `ErrorCode::invalid_value` if the path does not exist or is not a
 * directory.
 */
struct ExistingDirectory {
  template <class Input>
  static constexpr bool accepts_input = Path::template accepts_input<Input>;

  template <class Input>
  using after_type = std::filesystem::path;

  template <class Prev>
  using storage_type = void;

  auto operator()(ActionCtx<void> ctx,
                  ActionResult<std::string_view> input) const
      -> ActionResult<std::filesystem::path> {
    if (!input.have_value()) return propagate<std::filesystem::path>(input);
    auto path = std::filesystem::path{input.value};
    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
      return ActionResult<std::filesystem::path>::fail(
          detail::invalid_value_error(ErrorKind::conversion, ctx.index,
                                      path.string(),
                                      "expected an existing directory"));
    }
    return ActionResult<std::filesystem::path>::ok(std::move(path));
  }
};

/**
 * @brief Converts a string token to type `T` via a user-supplied mapper
 * function.
 *
 * `Mapper` must be a constexpr callable of the form
 * `(std::string_view) -> std::optional<T>`. Returns `ErrorCode::invalid_choice`
 * when the mapper returns `std::nullopt`.
 *
 * @tparam T      The target value type.
 * @tparam Mapper A constexpr callable mapping strings to `std::optional<T>`.
 */
template <class T, auto Mapper>
struct Choice {
  template <class Input>
  static constexpr bool accepts_input = Bool::template accepts_input<Input>;

  template <class Input>
  using after_type = T;

  template <class Prev>
  using storage_type = void;

  auto operator()(ActionCtx<void> ctx,
                  ActionResult<std::string_view> input) const
      -> ActionResult<T> {
    if (!input.have_value()) return propagate<T>(input);
    if (auto value = Mapper(input.value); value.has_value()) {
      return ActionResult<T>::ok(*std::move(value));
    }
    return ActionResult<T>::fail(
        detail::invalid_choice_error(ctx.index, std::string(input.value)));
  }
};

template <class T, auto Mapper>
using Enumeration = Choice<T, Mapper>;

template <std::integral T>
inline constexpr auto integer = Action<Integer<T>{}>{};
template <std::floating_point T>
inline constexpr auto floating = Action<Floating<T>{}>{};
template <class T, auto Mapper>
inline constexpr auto choice = Action<Choice<T, Mapper>{}>{};
template <class T, auto Mapper>
inline constexpr auto enumeration = Action<Enumeration<T, Mapper>{}>{};
/**
 * @brief Result of the `negatable<>` conversion.
 *
 * Supports structured bindings and satisfies `PairLike` so it can be used
 * directly with `pack::insert_or_assign`.
 */
struct NegatableResult {
  std::string name;
  bool enabled;
};

/**
 * @brief Converts a string token to `NegatableResult` by detecting a
 * compile-time negation prefix.
 *
 * Given prefix `"no-"`:
 * - `"lto"` → `{"lto", true}`
 * - `"no-lto"` → `{"lto", false}`
 *
 * @tparam Prefix The negation prefix string (e.g. `"no-"`, `"!"`).
 */
template <StringLiteral Prefix>
struct Negatable {
  template <class Input>
  static constexpr bool accepts_input =
      std::same_as<std::remove_cvref_t<Input>, std::string_view> ||
      std::same_as<std::remove_cvref_t<Input>, std::string>;

  template <class Input>
  using after_type = NegatableResult;

  template <class Prev>
  using storage_type = void;

  constexpr auto operator()(ActionCtx<void>,
                            ActionResult<std::string_view> input) const
      -> ActionResult<NegatableResult> {
    if (!input.have_value()) return propagate<NegatableResult>(input);
    constexpr std::string_view prefix = Prefix.view();
    const std::string_view val = input.value;
    if (val.starts_with(prefix)) {
      return ActionResult<NegatableResult>::ok(
          NegatableResult{std::string(val.substr(prefix.size())), false});
    }
    return ActionResult<NegatableResult>::ok(
        NegatableResult{std::string(val), true});
  }
};

template <StringLiteral Prefix>
inline constexpr auto negatable = Action<Negatable<Prefix>{}>{};
inline constexpr auto string = Action<String{}>{};
inline constexpr auto boolean = Action<Bool{}>{};
inline constexpr auto path = Action<Path{}>{};
inline constexpr auto existing_file = Action<ExistingFile{}>{};
inline constexpr auto existing_directory = Action<ExistingDirectory{}>{};

/**
 * @brief Injects a compile-time default value when an option is invoked as a
 * flag (no value provided).
 *
 * Accepts `std::optional<std::string_view>` as input:
 * - If the optional has a value (actual value provided), unwrap and pass
 * through.
 * - If nullopt (flag-only invocation) and `nargs == 0`, inject the compile-time
 *   default string.
 * - If nullopt but nargs > 0 (value coming separately), return ignore.
 *
 * @tparam Value The default string value to inject when invoked as a flag.
 */
template <StringLiteral Value>
struct DefaultMissingValue {
  template <class Input>
  static constexpr bool accepts_input =
      std::same_as<std::remove_cvref_t<Input>, std::optional<std::string_view>>;

  template <class Input>
  using after_type = std::string_view;

  template <class Prev>
  using storage_type = void;

  auto operator()(ActionCtx<void> ctx,
                  ActionResult<std::optional<std::string_view>> input) const
      -> ActionResult<std::string_view> {
    if (!input.have_value()) return propagate<std::string_view>(input);
    if (input.value.has_value()) {
      // actual value provided: unwrap and pass through
      return ActionResult<std::string_view>::ok(*input.value);
    }
    // nullopt: flag invocation
    if (ctx.nargs == 0) {
      // pure flag with no value: inject the compile-time default
      return ActionResult<std::string_view>::ok(Value.view());
    }
    // flag seen but a value is coming via separate value invocation: ignore
    return ActionResult<std::string_view>::ignore();
  }
};

template <StringLiteral Value>
inline constexpr auto default_missing_value =
    Action<DefaultMissingValue<Value>{}>{};

}  // namespace conversion

/**
 * @brief Actions that validate a typed value after conversion.
 *
 * Each action in this namespace passes its input through unchanged on success
 * or returns `ErrorCode::validation_failed` / `ErrorCode::invalid_choice` on
 * failure. They are placed after a conversion action and before a pack action:
 *
 * @code
 *   constexpr auto my_action = cli::conversion::integer<int>
 *                            | cli::validation::range<1, 100>
 *                            | cli::pack::set_once;
 * @endcode
 */
namespace validation {

/**
 * @brief Validates that the input value is greater than or equal to `MinValue`.
 *
 * @tparam MinValue The inclusive lower bound (non-type template parameter).
 */
template <auto MinValue>
struct Min {
  template <class Prev>
  static constexpr bool accepts_input =
      std::totally_ordered<std::remove_cvref_t<Prev>>;

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = void;

  template <class U>
  auto operator()(ActionCtx<void> ctx, ActionResult<U> input) const
      -> ActionResult<U> {
    if (!input.have_value()) return input;
    if (input.value < MinValue) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, detail::to_error_subject(input.value),
          std::format("value must be >= {}", MinValue)));
    }
    return input;
  }
};

/**
 * @brief Validates that the input value is less than or equal to `MaxValue`.
 *
 * @tparam MaxValue The inclusive upper bound (non-type template parameter).
 */
template <auto MaxValue>
struct Max {
  template <class Prev>
  static constexpr bool accepts_input =
      std::totally_ordered<std::remove_cvref_t<Prev>>;

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = void;

  template <class U>
  auto operator()(ActionCtx<void> ctx, ActionResult<U> input) const
      -> ActionResult<U> {
    if (!input.have_value()) return input;
    if (input.value > MaxValue) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, detail::to_error_subject(input.value),
          std::format("value must be <= {}", MaxValue)));
    }
    return input;
  }
};

/**
 * @brief Validates that the input value is within the closed interval [MinValue,
 * MaxValue].
 *
 * @tparam MinValue Inclusive lower bound.
 * @tparam MaxValue Inclusive upper bound. Must be the same type as `MinValue`.
 */
template <auto MinValue, auto MaxValue>
  requires std::is_same_v<decltype(MinValue), decltype(MaxValue)>
struct Range {
  template <class Prev>
  static constexpr bool accepts_input =
      std::totally_ordered<std::remove_cvref_t<Prev>>;

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = void;

  template <class U>
  auto operator()(ActionCtx<void> ctx, ActionResult<U> input) const
      -> ActionResult<U> {
    if (!input.have_value()) return input;
    if (input.value < MinValue || input.value > MaxValue) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, detail::to_error_subject(input.value),
          std::format("value must be between {} and {}", MinValue, MaxValue)));
    }
    return input;
  }
};

/** @brief Validates that the input value is strictly positive (> 0). */
struct Positive {
  template <class Prev>
  static constexpr bool accepts_input =
      std::totally_ordered<detail::decay_t<Prev>>;

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = void;

  template <class U>
  auto operator()(ActionCtx<void> ctx, ActionResult<U> input) const
      -> ActionResult<U> {
    if (!input.have_value()) return input;
    if (!(input.value > 0)) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, detail::to_error_subject(input.value),
          "value must be positive"));
    }
    return input;
  }
};

/** @brief Validates that the input value is non-negative (>= 0). */
struct NonNegative {
  template <class Prev>
  static constexpr bool accepts_input =
      std::totally_ordered<detail::decay_t<Prev>>;

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = void;

  template <class U>
  auto operator()(ActionCtx<void> ctx, ActionResult<U> input) const
      -> ActionResult<U> {
    if (!input.have_value()) return input;
    if (input.value < 0) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, detail::to_error_subject(input.value),
          "value must be non-negative"));
    }
    return input;
  }
};

/** @brief Validates that the input value is not empty (requires `.empty()`
 * member). */
struct NonEmpty {
  template <class Prev>
  static constexpr bool accepts_input =
      requires(const detail::decay_t<Prev>& value) { value.empty(); };

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = void;

  template <class U>
  auto operator()(ActionCtx<void> ctx, ActionResult<U> input) const
      -> ActionResult<U> {
    if (!input.have_value()) return input;
    if (input.value.empty()) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, detail::to_error_subject(input.value),
          "value must not be empty"));
    }
    return input;
  }
};

/** @brief Validates that the input string is not blank (not all whitespace). */
struct NotBlank {
  template <class Prev>
  static constexpr bool accepts_input =
      detail::StringLike<Prev> ||
      std::same_as<detail::decay_t<Prev>, const char*>;

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = void;

  template <class U>
  auto operator()(ActionCtx<void> ctx, ActionResult<U> input) const
      -> ActionResult<U> {
    if (!input.have_value()) return input;
    std::string_view text = input.value;
    if (detail::is_blank(text)) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, std::string(text), "value must not be blank"));
    }
    return input;
  }
};

/**
 * @brief Validates that the input value equals one of the compile-time constants
 * `Allowed`.
 *
 * @tparam Allowed Non-type template pack of allowed values. All must be the same
 * type.
 */
template <auto... Allowed>
struct OneOf {
  template <class Prev>
  static constexpr bool accepts_input =
      ((std::same_as<detail::decay_t<Prev>,
                     detail::decay_t<decltype(Allowed)>>) &&
       ...);

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = void;

  template <class U>
  auto operator()(ActionCtx<void> ctx, ActionResult<U> input) const
      -> ActionResult<U> {
    if (!input.have_value()) return input;
    if (((input.value == Allowed) || ...)) {
      return input;
    }
    return ActionResult<U>::fail(detail::validation_failed_error(
        ctx.index, detail::to_error_subject(input.value),
        "value was not one of the allowed choices"));
  }
};

/**
 * @brief Validates that the input string matches a compile-time regular
 * expression.
 *
 * Uses `std::regex_match` (full-match semantics). Returns
 * `ErrorCode::validation_failed` when the string does not match.
 *
 * @tparam Pattern The regex pattern string (e.g. `"[a-z]+"`).
 */
template <StringLiteral Pattern>
struct Matches {
  template <class Prev>
  static constexpr bool accepts_input = detail::StringLike<Prev>;

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = void;

  template <class U>
  auto operator()(ActionCtx<void> ctx, ActionResult<U> input) const
      -> ActionResult<U> {
    if (!input.have_value()) return input;
    const auto regex = std::regex{std::string(Pattern.view())};
    if (!std::regex_match(input.value.begin(), input.value.end(), regex)) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, detail::to_error_subject(input.value),
          std::format("value must match {}", Pattern.view())));
    }
    return input;
  }
};

/** @brief Validates that a `std::filesystem::path` exists on the filesystem. */
struct Exists {
  template <class Prev>
  static constexpr bool accepts_input =
      std::same_as<detail::decay_t<Prev>, std::filesystem::path>;

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = void;

  template <class U>
  auto operator()(ActionCtx<void> ctx, ActionResult<U> input) const
      -> ActionResult<U> {
    if (!input.have_value()) return input;
    if (!std::filesystem::exists(input.value)) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, input.value.string(), "path does not exist"));
    }
    return input;
  }
};

/** @brief Validates that a `std::filesystem::path` refers to a regular file. */
struct IsRegularFile {
  template <class Prev>
  static constexpr bool accepts_input =
      std::same_as<detail::decay_t<Prev>, std::filesystem::path>;

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = void;

  template <class U>
  auto operator()(ActionCtx<void> ctx, ActionResult<U> input) const
      -> ActionResult<U> {
    if (!input.have_value()) return input;
    if (!std::filesystem::is_regular_file(input.value)) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, input.value.string(), "path is not a regular file"));
    }
    return input;
  }
};

/** @brief Validates that a `std::filesystem::path` refers to a directory. */
struct IsDirectory {
  template <class Prev>
  static constexpr bool accepts_input =
      std::same_as<detail::decay_t<Prev>, std::filesystem::path>;

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = void;

  template <class U>
  auto operator()(ActionCtx<void> ctx, ActionResult<U> input) const
      -> ActionResult<U> {
    if (!input.have_value()) return input;
    if (!std::filesystem::is_directory(input.value)) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, input.value.string(), "path is not a directory"));
    }
    return input;
  }
};

/** @brief Validates that the parent directory of a `std::filesystem::path`
 * exists. */
struct ParentExists {
  template <class Prev>
  static constexpr bool accepts_input =
      std::same_as<detail::decay_t<Prev>, std::filesystem::path>;

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = void;

  template <class U>
  auto operator()(ActionCtx<void> ctx, ActionResult<U> input) const
      -> ActionResult<U> {
    if (!input.have_value()) return input;
    const auto parent = input.value.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent)) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, input.value.string(), "parent directory does not exist"));
    }
    return input;
  }
};

/**
 * @brief Validates a value using a user-supplied compile-time predicate.
 *
 * `Pred` must be a constexpr callable of the form `(const T&) -> bool`.
 * Returns `ErrorCode::validation_failed` when `Pred` returns `false`.
 *
 * @tparam Pred A constexpr callable acting as the validation predicate.
 */
template <auto Pred>
struct Predicate {
  template <class Prev>
  static constexpr bool accepts_input =
      requires(const detail::decay_t<Prev>& value) {
        { Pred(value) } -> std::convertible_to<bool>;
      };

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = void;

  template <class U>
  auto operator()(ActionCtx<void> ctx, ActionResult<U> input) const
      -> ActionResult<U> {
    if (!input.have_value()) return input;
    if (!Pred(input.value)) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, detail::to_error_subject(input.value),
          "predicate rejected value"));
    }
    return input;
  }
};

template <auto MinValue>
inline constexpr auto min = Action<Min<MinValue>{}>{};
template <auto MaxValue>
inline constexpr auto max = Action<Max<MaxValue>{}>{};
template <auto MinValue, auto MaxValue>
inline constexpr auto range = Action<Range<MinValue, MaxValue>{}>{};
template <auto... Allowed>
inline constexpr auto one_of = Action<OneOf<Allowed...>{}>{};
template <StringLiteral Pattern>
inline constexpr auto matches = Action<Matches<Pattern>{}>{};
template <auto Pred>
inline constexpr auto predicate = Action<Predicate<Pred>{}>{};
inline constexpr auto positive = Action<Positive{}>{};
inline constexpr auto non_negative = Action<NonNegative{}>{};
inline constexpr auto non_empty = Action<NonEmpty{}>{};
inline constexpr auto not_blank = Action<NotBlank{}>{};
inline constexpr auto exists = Action<Exists{}>{};
inline constexpr auto is_regular_file = Action<IsRegularFile{}>{};
inline constexpr auto is_directory = Action<IsDirectory{}>{};
inline constexpr auto parent_exists = Action<ParentExists{}>{};

}  // namespace validation

/**
 * @brief Terminal actions that write the converted (and validated) value into
 * argument storage.
 *
 * Pack actions are always the last step in an action pipeline. They define the
 * `storage_type` alias that determines the type of the field's `.value()`
 * member.
 *
 * Example:
 * @code
 *   constexpr auto my_action = cli::conversion::integer<int> | cli::pack::push;
 * @endcode
 */
namespace pack {

/** @brief Sets the `bool` storage to `true`. Used as the action for `Flag`. */
struct SetTrue {
  template <class Prev>
  static constexpr bool accepts_input = true;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = bool;

  template <class T>
  auto operator()(ActionCtx<bool> ctx, ActionResult<T> input) const
      -> ActionResult<void> {
    if (!input.have_value()) return propagate<void>(input);
    ctx.arg.get() = true;
    return ActionResult<void>::ok();
  }
};

/** @brief Sets the `bool` storage to `false`. */
struct SetFalse {
  template <class Prev>
  static constexpr bool accepts_input = true;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = bool;

  template <class T>
  auto operator()(ActionCtx<bool> ctx, ActionResult<T> input) const
      -> ActionResult<void> {
    if (!input.have_value()) return propagate<void>(input);
    ctx.arg.get() = false;
    return ActionResult<void>::ok();
  }
};

/** @brief Flips the `bool` storage each time the option is seen. */
struct Toggle {
  template <class Prev>
  static constexpr bool accepts_input = true;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = bool;

  template <class T>
  auto operator()(ActionCtx<bool> ctx, ActionResult<T> input) const
      -> ActionResult<void> {
    if (!input.have_value()) return propagate<void>(input);
    ctx.arg.get() = !ctx.arg.get();
    return ActionResult<void>::ok();
  }
};

/** @brief Increments a `std::size_t` counter each time the option is seen. */
struct Increment {
  template <class Prev>
  static constexpr bool accepts_input = true;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = std::size_t;

  template <class T>
  auto operator()(ActionCtx<std::size_t> ctx, ActionResult<T> input) const
      -> ActionResult<void> {
    if (!input.have_value()) return propagate<void>(input);
    ++ctx.arg.get();
    return ActionResult<void>::ok();
  }
};

/**
 * @brief Passes the value through unchanged, but returns
 * `ErrorCode::duplicate_argument` if the option has been seen more than once.
 *
 * This is a *filter* action (non-terminal); it must be followed by a terminal
 * pack action.
 */
struct RejectDuplicate {
  template <class Prev>
  static constexpr bool accepts_input =
      !std::same_as<detail::decay_t<Prev>, void>;

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = void;

  template <class T>
  auto operator()(ActionCtx<void> ctx, ActionResult<T> input) const
      -> ActionResult<T> {
    if (!input.have_value()) return input;
    if (ctx.occurrences > 1) {
      return ActionResult<T>::fail(detail::duplicate_argument_error(
          ctx.index, detail::to_error_subject(input.value)));
    }
    return input;
  }
};

/**
 * @brief Stores the value in `std::optional<T>` and rejects subsequent
 * occurrences.
 *
 * Storage type: `std::optional<T>`. Returns `ErrorCode::duplicate_argument` if
 * the option appears more than once. This is the default terminal action for
 * `Option<T, ...>`.
 */
struct SetOnce {
  template <class Prev>
  static constexpr bool accepts_input =
      !std::same_as<detail::decay_t<Prev>, void>;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = std::optional<detail::decay_t<Prev>>;

  template <class T>
  auto operator()(ActionCtx<std::optional<detail::decay_t<T>>> ctx,
                  ActionResult<T> input) const -> ActionResult<void> {
    if (!input.have_value()) return propagate<void>(input);
    if (ctx.arg.get().has_value() || ctx.occurrences > 1) {
      return ActionResult<void>::fail(detail::duplicate_argument_error(
          ctx.index, detail::to_error_subject(input.value)));
    }
    ctx.arg.get() = std::move(input.value);
    return ActionResult<void>::ok();
  }
};

/**
 * @brief Appends the value to a `std::vector<T>`, rejecting duplicate values.
 *
 * Storage type: `std::vector<T>`. Returns `ErrorCode::duplicate_argument` if
 * the same value is pushed more than once.
 */
struct PushUnique {
  template <class Prev>
  static constexpr bool accepts_input =
      !std::same_as<detail::decay_t<Prev>, void>;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = std::vector<detail::decay_t<Prev>>;

  template <class T>
  auto operator()(ActionCtx<std::vector<detail::decay_t<T>>> ctx,
                  ActionResult<T> input) const -> ActionResult<void> {
    if (!input.have_value()) return propagate<void>(input);
    auto& values = ctx.arg.get();
    if (std::ranges::find(values, input.value) != values.end()) {
      return ActionResult<void>::fail(detail::duplicate_argument_error(
          ctx.index, detail::to_error_subject(input.value)));
    }
    values.push_back(std::move(input.value));
    return ActionResult<void>::ok();
  }
};

/**
 * @brief Appends the value to a `std::vector<T>`.
 *
 * Storage type: `std::vector<T>`. This is the default terminal action for
 * `ListOption<T, ...>`.
 */
struct Push {
  template <class Prev>
  static constexpr bool accepts_input =
      !std::same_as<detail::decay_t<Prev>, void>;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = std::vector<detail::decay_t<Prev>>;

  template <class T>
  auto operator()(ActionCtx<std::vector<detail::decay_t<T>>> ctx,
                  ActionResult<T> input) const -> ActionResult<void> {
    if (!input.have_value()) return propagate<void>(input);
    ctx.arg.get().push_back(std::move(input.value));
    return ActionResult<void>::ok();
  }
};

/**
 * @brief Inserts the value into a `std::set<T>` (duplicates are silently ignored
 * by the set).
 *
 * Storage type: `std::set<T>`. Requires the value type to be totally ordered.
 */
struct Insert {
  template <class Prev>
  static constexpr bool accepts_input =
      !std::same_as<detail::decay_t<Prev>, void> &&
      std::totally_ordered<detail::decay_t<Prev>>;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = std::set<detail::decay_t<Prev>>;

  template <class T>
  auto operator()(ActionCtx<std::set<detail::decay_t<T>>> ctx,
                  ActionResult<T> input) const -> ActionResult<void> {
    if (!input.have_value()) return propagate<void>(input);
    ctx.arg.get().insert(std::move(input.value));
    return ActionResult<void>::ok();
  }
};

struct InsertOrAssign {
  template <class Prev>
  static constexpr bool accepts_input = detail::PairLike<Prev>;

  template <class Prev>
  using after_type = void;

  template <class Prev, bool = detail::PairLike<Prev>>
  struct storage_type_impl {
    using type = void;
  };
  template <class Prev>
  struct storage_type_impl<Prev, true> {
    using type = std::map<
        std::remove_cvref_t<std::tuple_element_t<0, detail::decay_t<Prev>>>,
        std::remove_cvref_t<std::tuple_element_t<1, detail::decay_t<Prev>>>>;
  };

  template <class Prev>
  using storage_type = typename storage_type_impl<Prev>::type;

  template <class T>
  auto operator()(ActionCtx<storage_type<T>> ctx, ActionResult<T> input) const
      -> ActionResult<void> {
    if (!input.have_value()) return propagate<void>(input);
    auto&& [key, value] = input.value;
    ctx.arg.get().insert_or_assign(key, value);
    return ActionResult<void>::ok();
  }
};

struct Extend {
  template <class Prev>
  static constexpr bool accepts_input =
      std::ranges::input_range<detail::decay_t<Prev>> &&
      !detail::StringLike<Prev>;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type =
      std::vector<std::ranges::range_value_t<detail::decay_t<Prev>>>;

  template <class T>
  auto operator()(ActionCtx<storage_type<T>> ctx, ActionResult<T> input) const
      -> ActionResult<void> {
    if (!input.have_value()) return propagate<void>(input);
    auto& out = ctx.arg.get();
    for (auto&& value : input.value) {
      out.emplace_back(value);
    }
    return ActionResult<void>::ok();
  }
};

/**
 * @brief Sets a `bool` flag to `true` when the option is seen, regardless of
 * value.
 *
 * Storage type: `bool`. Useful for detecting option presence without caring
 * about the value of the option.
 */
struct MarkPresent {
  template <class Prev>
  static constexpr bool accepts_input = true;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = bool;

  template <class T>
  auto operator()(ActionCtx<bool> ctx, ActionResult<T> input) const
      -> ActionResult<void> {
    if (!input.have_value()) return propagate<void>(input);
    ctx.arg.get() = true;
    return ActionResult<void>::ok();
  }
};

/**
 * @brief Writes the value directly into an external variable via a `T*` pointer.
 *
 * Storage type: `T*`. The pointer must be non-null at the time of parsing;
 * returns `ErrorCode::validation_failed` with a descriptive message if the
 * pointer is null. Used as the action for `BoundOption`.
 *
 * @tparam T The type of the external variable.
 */
template <class T>
struct StoreInto {
  template <class Prev>
  static constexpr bool accepts_input = std::same_as<detail::decay_t<Prev>, T>;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = T*;

  auto operator()(ActionCtx<T*> ctx, ActionResult<T> input) const
      -> ActionResult<void> {
    if (!input.have_value()) return propagate<void>(input);
    if (T* ptr = ctx.arg.get(); ptr != nullptr) {
      *ptr = std::move(input.value);
      return ActionResult<void>::ok();
    }
    return ActionResult<void>::fail(detail::validation_failed_error(
        ctx.index, "store_into",
        "target pointer is null; did you forget to call bind()?"));
  }
};

/**
 * @brief Invokes a compile-time callable `Fn` with the parsed value as a
 * terminal action.
 *
 * `Fn` may have any of the following signatures:
 * - `(ActionCtx<void>, T value)` — receives context and value.
 * - `(T value)` — receives value only.
 * - `()` — receives nothing.
 *
 * Storage type: `std::monostate` (no value is stored; the callback is the only
 * effect).
 *
 * @tparam Fn A constexpr callable to invoke.
 */
template <auto Fn>
struct Callback {
  template <class Prev>
  static constexpr bool accepts_input = true;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = std::monostate;

  template <class T>
  auto operator()(ActionCtx<std::monostate> ctx, ActionResult<T> input) const
      -> ActionResult<void> {
    if (!input.have_value()) return propagate<void>(input);
    if constexpr (requires { Fn(ctx, input.value); }) {
      Fn(ctx, input.value);
    } else if constexpr (requires { Fn(input.value); }) {
      Fn(input.value);
    } else {
      Fn();
    }
    return ActionResult<void>::ok();
  }
};

inline constexpr auto set_true = Action<SetTrue{}>{};
inline constexpr auto set_false = Action<SetFalse{}>{};
inline constexpr auto toggle = Action<Toggle{}>{};
inline constexpr auto increment = Action<Increment{}>{};
inline constexpr auto set_once = Action<SetOnce{}>{};
inline constexpr auto reject_duplicate = Action<RejectDuplicate{}>{};
inline constexpr auto push_unique = Action<PushUnique{}>{};
inline constexpr auto push = Action<Push{}>{};
inline constexpr auto insert = Action<Insert{}>{};
inline constexpr auto insert_or_assign = Action<InsertOrAssign{}>{};
// inline constexpr auto extend = Action<Extend{}>{};
inline constexpr auto mark_present = Action<MarkPresent{}>{};
template <auto Fn>
inline constexpr auto callback = Action<Callback<Fn>{}>{};
template <class T>
inline constexpr auto store_into = Action<StoreInto<T>{}>{};

}  // namespace pack

/**
 * @brief Actions that control the parser's control flow (help, exit).
 */
namespace action {

/**
 * @brief Wraps the current value in a `HelpRequested<T>` sentinel type.
 *
 * When `ExitSuccess` follows this action, it detects the `HelpRequested` wrapper
 * and emits `ErrorCode::help_requested` (which instructs the parser to print
 * help and exit 0).
 */
struct PrintHelp {
  template <class Prev>
  static constexpr bool accepts_input = true;

  template <class Prev>
  using after_type = detail::HelpRequested<Prev>;

  template <class Prev>
  using storage_type = void;

  template <class Arg, class T>
  auto operator()(ActionCtx<Arg>, ActionResult<T> input) const
      -> ActionResult<after_type<T>> {
    if (!input.have_value()) return propagate<after_type<T>>(input);
    return ActionResult<after_type<T>>::ok(
        after_type<T>{.value = std::move(input.value)});
  }
};

/**
 * @brief Terminates the pipeline by returning a `ParseError` with code
 * `exit_success` (or `help_requested` when preceded by `PrintHelp`).
 *
 * The parser catches these special codes and either prints help text or exits
 * cleanly with code 0, without treating them as real errors.
 */
struct ExitSuccess {
  template <class Prev>
  static constexpr bool accepts_input = true;

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = std::monostate;

  template <class Arg, class T>
  auto operator()(ActionCtx<Arg> ctx, ActionResult<T> input) const
      -> ActionResult<T> {
    if (!input.have_value()) return input;
    using U = std::remove_cvref_t<T>;
    if constexpr (detail::IsHelpRequested<U>::value) {
      return ActionResult<T>::fail(ParseError{
          .code = ErrorCode::help_requested,
          .kind = ErrorKind::parse,
          .position = static_cast<int>(ctx.index),
      });
    } else {
      return ActionResult<T>::fail(ParseError{
          .code = ErrorCode::exit_success,
          .kind = ErrorKind::parse,
          .position = static_cast<int>(ctx.index),
      });
    }
  }
};

inline constexpr auto print_help = Action<PrintHelp{}>{};
inline constexpr auto exit_success = Action<ExitSuccess{}>{};

}  // namespace action

}  // namespace cli

// Tuple protocol for NegatableResult — required for PairLike and structured
// bindings with pack::insert_or_assign.
template <>
struct std::tuple_size<cli::conversion::NegatableResult>
    : std::integral_constant<std::size_t, 2> {};
template <>
struct std::tuple_element<0, cli::conversion::NegatableResult> {
  using type = std::string;
};
template <>
struct std::tuple_element<1, cli::conversion::NegatableResult> {
  using type = bool;
};

namespace cli::conversion {

template <std::size_t I>
auto get(NegatableResult& r) -> decltype(auto) {
  if constexpr (I == 0)
    return (r.name);
  else
    return (r.enabled);
}
template <std::size_t I>
auto get(const NegatableResult& r) -> decltype(auto) {
  if constexpr (I == 0)
    return (r.name);
  else
    return (r.enabled);
}

}  // namespace cli::conversion
