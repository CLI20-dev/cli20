#pragma once

#include <algorithm>
#include <cctype>
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

template <class T>
struct ActionResult {
  using value_type = T;

  ParseError error{};
  T value{};

  [[nodiscard]]
  constexpr explicit operator bool() const noexcept {
    return !error.has_error();
  }

  [[nodiscard]]
  constexpr auto has_value() const noexcept -> bool {
    return !error.has_error();
  }

  [[nodiscard]]
  constexpr auto has_error() const noexcept -> bool {
    return error.has_error();
  }

  template <class U>
  [[nodiscard]]
  static constexpr auto ok(U&& value) -> ActionResult<T> {
    return {
        .error = {},
        .value = std::forward<U>(value),
    };
  }

  [[nodiscard]]
  static constexpr auto fail(ParseError error) -> ActionResult<T> {
    return {
        .error = std::move(error),
        .value = {},
    };
  }
};

template <>
struct ActionResult<void> {
  ParseError error{};

  [[nodiscard]]
  constexpr explicit operator bool() const noexcept {
    return !error.has_error();
  }

  [[nodiscard]]
  constexpr auto has_value() const noexcept -> bool {
    return !error.has_error();
  }

  [[nodiscard]]
  constexpr auto has_error() const noexcept -> bool {
    return error.has_error();
  }

  static constexpr auto ok() -> ActionResult<void> {
    return {
        .error = {},
    };
  }

  [[nodiscard]]
  static constexpr auto fail(ParseError error) -> ActionResult<void> {
    return {
        .error = std::move(error),
    };
  }
};

template <class T = void>
struct ActionCtx {
  size_t index{};
  size_t occurrences{};
  size_t invoke_count{};
  std::reference_wrapper<T> arg{};
};

template <>
struct ActionCtx<void> {
  size_t index{};
  size_t occurrences{};
  size_t invoke_count{};

  template <class T>
  ActionCtx(const ActionCtx<T>& other)
      : index(other.index),
        occurrences(other.occurrences),
        invoke_count(other.invoke_count) {}

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

template <auto... Fns>
  requires(requires {
    {
      std::remove_cvref_t<decltype(Fns)>::template accepts_input<void>
    } -> std::convertible_to<bool>;
    typename std::remove_cvref_t<decltype(Fns)>::template after_type<void>;
    typename std::remove_cvref_t<decltype(Fns)>::template storage_type<void>;
  } && ...)
struct Action {
  template <class Arg, class Result>
  [[nodiscard]]
  static constexpr auto invoke(ActionCtx<Arg>& ctx, ActionResult<Result> input) {
    return
        []<auto FnHead, auto... FnTail>(
            ActionCtx<Arg>& ctx, ActionResult<Result> input) -> decltype(auto) {
          using Head = std::remove_cvref_t<decltype(FnHead)>;
          using Next = typename Head::template after_type<Result>;

          if (!input) {
            if constexpr (sizeof...(FnTail) == 0) {
              return ActionResult<Next>::fail(input.error);
            } else {
              return Action<FnTail...>::invoke(
                  ctx, ActionResult<Next>::fail(input.error));
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

      if (!input) {
        if constexpr (sizeof...(FnTail) == 0) {
          return ActionResult<Next>::fail(input.error);
        } else {
          return Action<FnTail...>::invoke(
              ctx, ActionResult<Next>::fail(input.error));
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

  static constexpr auto validate() -> auto {
    return validate_impl<std::string_view, Fns...>();
  }
};

template <>
struct Action<> {
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

namespace conversion {

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
    return ActionResult<std::string>::ok(std::string(input.value));
  }
};

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
    return ActionResult<std::filesystem::path>::ok(
        std::filesystem::path{input.value});
  }
};

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
inline constexpr auto string = Action<String{}>{};
inline constexpr auto boolean = Action<Bool{}>{};
inline constexpr auto path = Action<Path{}>{};
inline constexpr auto existing_file = Action<ExistingFile{}>{};
inline constexpr auto existing_directory = Action<ExistingDirectory{}>{};

}  // namespace conversion

namespace validation {

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
    if (input.value < MinValue) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, detail::to_error_subject(input.value),
          std::format("value must be >= {}", MinValue)));
    }
    return input;
  }
};

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
    if (input.value > MaxValue) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, detail::to_error_subject(input.value),
          std::format("value must be <= {}", MaxValue)));
    }
    return input;
  }
};

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
    if (input.value < MinValue || input.value > MaxValue) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, detail::to_error_subject(input.value),
          std::format("value must be between {} and {}", MinValue, MaxValue)));
    }
    return input;
  }
};

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
    if (!(input.value > 0)) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, detail::to_error_subject(input.value),
          "value must be positive"));
    }
    return input;
  }
};

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
    if (input.value < 0) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, detail::to_error_subject(input.value),
          "value must be non-negative"));
    }
    return input;
  }
};

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
    if (input.value.empty()) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, detail::to_error_subject(input.value),
          "value must not be empty"));
    }
    return input;
  }
};

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
    std::string_view text = input.value;
    if (detail::is_blank(text)) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, std::string(text), "value must not be blank"));
    }
    return input;
  }
};

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
    if (((input.value == Allowed) || ...)) {
      return input;
    }
    return ActionResult<U>::fail(detail::validation_failed_error(
        ctx.index, detail::to_error_subject(input.value),
        "value was not one of the allowed choices"));
  }
};

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
    const auto regex = std::regex{std::string(Pattern.view())};
    if (!std::regex_match(input.value.begin(), input.value.end(), regex)) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, detail::to_error_subject(input.value),
          std::format("value must match {}", Pattern.view())));
    }
    return input;
  }
};

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
    if (!std::filesystem::exists(input.value)) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, input.value.string(), "path does not exist"));
    }
    return input;
  }
};

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
    if (!std::filesystem::is_regular_file(input.value)) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, input.value.string(), "path is not a regular file"));
    }
    return input;
  }
};

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
    if (!std::filesystem::is_directory(input.value)) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, input.value.string(), "path is not a directory"));
    }
    return input;
  }
};

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
    const auto parent = input.value.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent)) {
      return ActionResult<U>::fail(detail::validation_failed_error(
          ctx.index, input.value.string(), "parent directory does not exist"));
    }
    return input;
  }
};

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

namespace pack {

struct SetTrue {
  template <class Prev>
  static constexpr bool accepts_input = true;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = bool;

  template <class T>
  auto operator()(ActionCtx<bool> ctx, ActionResult<T>) const
      -> ActionResult<void> {
    ctx.arg.get() = true;
    return ActionResult<void>::ok();
  }
};

struct SetFalse {
  template <class Prev>
  static constexpr bool accepts_input = true;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = bool;

  template <class T>
  auto operator()(ActionCtx<bool> ctx, ActionResult<T>) const
      -> ActionResult<void> {
    ctx.arg.get() = false;
    return ActionResult<void>::ok();
  }
};

struct Toggle {
  template <class Prev>
  static constexpr bool accepts_input = true;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = bool;

  template <class T>
  auto operator()(ActionCtx<bool> ctx, ActionResult<T>) const
      -> ActionResult<void> {
    ctx.arg.get() = !ctx.arg.get();
    return ActionResult<void>::ok();
  }
};

struct Increment {
  template <class Prev>
  static constexpr bool accepts_input = true;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = std::size_t;

  template <class T>
  auto operator()(ActionCtx<std::size_t> ctx, ActionResult<T>) const
      -> ActionResult<void> {
    ++ctx.arg.get();
    return ActionResult<void>::ok();
  }
};

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
    if (ctx.occurrences > 1) {
      return ActionResult<T>::fail(detail::duplicate_argument_error(
          ctx.index, detail::to_error_subject(input.value)));
    }
    return input;
  }
};

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
    if (ctx.arg.get().has_value() || ctx.occurrences > 1) {
      return ActionResult<void>::fail(detail::duplicate_argument_error(
          ctx.index, detail::to_error_subject(input.value)));
    }
    ctx.arg.get() = std::move(input.value);
    return ActionResult<void>::ok();
  }
};

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
    auto& values = ctx.arg.get();
    if (std::ranges::find(values, input.value) != values.end()) {
      return ActionResult<void>::fail(detail::duplicate_argument_error(
          ctx.index, detail::to_error_subject(input.value)));
    }
    values.push_back(std::move(input.value));
    return ActionResult<void>::ok();
  }
};

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
    ctx.arg.get().push_back(std::move(input.value));
    return ActionResult<void>::ok();
  }
};

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
    ctx.arg.get().insert(std::move(input.value));
    return ActionResult<void>::ok();
  }
};

struct InsertOrAssign {
  template <class Prev>
  static constexpr bool accepts_input = detail::PairLike<Prev>;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = std::map<
      std::remove_cvref_t<std::tuple_element_t<0, detail::decay_t<Prev>>>,
      std::remove_cvref_t<std::tuple_element_t<1, detail::decay_t<Prev>>>>;

  template <class T>
  auto operator()(ActionCtx<storage_type<T>> ctx, ActionResult<T> input) const
      -> ActionResult<void> {
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
    auto& out = ctx.arg.get();
    for (auto&& value : input.value) {
      out.emplace_back(value);
    }
    return ActionResult<void>::ok();
  }
};

struct MarkPresent {
  template <class Prev>
  static constexpr bool accepts_input = true;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = bool;

  template <class T>
  auto operator()(ActionCtx<bool> ctx, ActionResult<T>) const
      -> ActionResult<void> {
    ctx.arg.get() = true;
    return ActionResult<void>::ok();
  }
};

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
    if (T* ptr = ctx.arg.get(); ptr != nullptr) {
      *ptr = std::move(input.value);
      return ActionResult<void>::ok();
    }
    return ActionResult<void>::fail(detail::validation_failed_error(
        ctx.index, "store_into",
        "target pointer is null; did you forget to call bind()?"));
  }
};

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
// inline constexpr auto insert_or_assign = Action<InsertOrAssign{}>{};
// inline constexpr auto extend = Action<Extend{}>{};
inline constexpr auto mark_present = Action<MarkPresent{}>{};
template <auto Fn>
inline constexpr auto callback = Action<Callback<Fn>{}>{};
template <class T>
inline constexpr auto store_into = Action<StoreInto<T>{}>{};

}  // namespace pack

namespace action {

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
    return ActionResult<after_type<T>>::ok(
        after_type<T>{.value = std::move(input.value)});
  }
};

struct ExitSuccess {
  template <class Prev>
  static constexpr bool accepts_input = true;

  template <class Prev>
  using after_type = Prev;

  template <class Prev>
  using storage_type = std::monostate;

  template <class Arg, class T>
  auto operator()(ActionCtx<Arg> ctx, ActionResult<T>) const -> ActionResult<T> {
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
