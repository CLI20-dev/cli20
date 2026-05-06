#pragma once

#include <charconv>
#include <concepts>
#include <cstddef>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "argon/error.hh"

namespace argon {

template <class T>
struct ActionResult {
  using value_type = T;

  ParseError error{};
  T value{};

  [[nodiscard]]
  constexpr explicit operator bool() const noexcept {
    return !error.hasError();
  }

  [[nodiscard]]
  constexpr auto has_value() const noexcept -> bool {
    return !error.hasError();
  }

  [[nodiscard]]
  constexpr auto has_error() const noexcept -> bool {
    return error.hasError();
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
    return !error.hasError();
  }

  [[nodiscard]]
  constexpr auto has_value() const noexcept -> bool {
    return !error.hasError();
  }

  [[nodiscard]]
  constexpr auto has_error() const noexcept -> bool {
    return error.hasError();
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
  size_t occurrences{};  // how many times the option token appeared on the CLI
  size_t
      invoke_count{};  // how many times invoke has been called for this option
  std::reference_wrapper<T> arg{};
};

template <>
struct ActionCtx<void> {
  size_t index{};
  size_t occurrences{};  // how many times the option token appeared on the CLI
  size_t
      invoke_count{};  // how many times invoke has been called for this option
  template <class T>
  ActionCtx(const ActionCtx<T>& other)
      : index(other.index),
        occurrences(other.occurrences),
        invoke_count(other.invoke_count) {}

  ActionCtx() = default;
};

template <class>
inline constexpr bool dependent_false_v = false;

template <auto Head, auto... Tail>
struct GetLast : GetLast<Tail...> {};

template <auto Last>
struct GetLast<Last> {
  static constexpr auto value = Last;
};

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
          if (!input) {
            using Next = typename std::remove_cvref_t<
                decltype(FnHead)>::template after_type<Result>;
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
            !std::same_as<typename decltype(FnHead)::template storage_type<Prev>,
                          void>,
        "The last action must have a non-void storage type");

    if constexpr (sizeof...(FnTail) == 0) {
      return std::make_pair(
          decltype(FnHead)::template accepts_input<Prev>,
          std::type_identity<
              typename decltype(FnHead)::template storage_type<Prev>>{});
    } else {
      if constexpr (!decltype(FnHead)::template accepts_input<Prev>) {
        return std::make_pair(false, std::type_identity<void>{});
      } else {
        using Next = typename decltype(FnHead)::template after_type<Prev>;
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
    T result = 0;
    std::from_chars_result r = std::from_chars(
        input.value.data(),
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        input.value.data() + input.value.size(), result);
    if (r.ec == std::errc::invalid_argument) {
      return ActionResult<T>::fail(
          ParseError{.code = ErrorCode::invalid_value,
                     .kind = ErrorKind::conversion,
                     .position = static_cast<int>(ctx.index),
                     .subject = std::string(input.value)});
    } else if (r.ec == std::errc::result_out_of_range) {
      return ActionResult<T>::fail(
          ParseError{.code = ErrorCode::out_of_range,
                     .kind = ErrorKind::conversion,
                     .position = static_cast<int>(ctx.index),
                     .subject = std::string(input.value)});
    }
    if (r.ec == std::errc{} &&
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        r.ptr != input.value.data() + input.value.size()) {
      return ActionResult<T>::fail(
          ParseError{.code = ErrorCode::invalid_value,
                     .kind = ErrorKind::conversion,
                     .position = static_cast<int>(ctx.index),
                     .subject = std::string(input.value),
                     .detail = "unexpected trailing characters"});
    }
    return ActionResult<T>::ok(result);
  };
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
  };
};

struct Bool {
  template <class Input>
  static constexpr bool accepts_input =
      std::same_as<std::remove_cvref_t<Input>, std::string_view> ||
      std::same_as<std::remove_cvref_t<Input>, std::string>;
  template <class Input>
  using after_type = std::string;
  template <class Prev>
  using storage_type = void;

  constexpr auto operator()(ActionCtx<void> ctx,
                            ActionResult<std::string_view> input) const
      -> ActionResult<bool> {
    const std::string_view val = input.value;
    if (val == "true" || val == "1") {
      return ActionResult<bool>::ok(true);
    } else if (val == "false" || val == "0") {
      return ActionResult<bool>::ok(false);
    } else {
      return ActionResult<bool>::fail(
          ParseError{.code = ErrorCode::invalid_value,
                     .kind = ErrorKind::conversion,
                     .position = static_cast<int>(ctx.index),
                     .subject = std::string(input.value),
                     .detail = "expected one of: true, false, 1, 0"});
    }
  };
};

template <std::integral T>
static constexpr auto integer = Integer<T>{};
static constexpr auto string = String{};
static constexpr auto boolean = Bool{};

};  // namespace conversion

namespace validation {

template <auto Min, auto Max>
  requires std::is_same_v<decltype(Min), decltype(Max)>
struct Range {
  using value_type = decltype(Min);
  template <class Prev>
  static constexpr bool accepts_input = std::is_convertible_v<Prev, value_type>;
  template <class Prev>
  using after_type = Prev;
  template <class Prev>
  using storage_type = void;

  template <class U>
  constexpr auto operator()(ActionCtx<void> ctx, ActionResult<U> input) const
      -> ActionResult<U> {
    if (input.value < Min || input.value > Max) {
      return ActionResult<U>::fail(ParseError{
          .code = ErrorCode::invalid_value,
          .kind = ErrorKind::validation,
          .position = static_cast<int>(ctx.index),
          .subject = std::to_string(input.value),
          .detail = std::format("value must be between {} and {}", Min, Max)});
    }
    return input;
  };
};

template <auto Min, auto Max>
static constexpr auto range = Range<Min, Max>{};

}  // namespace validation

namespace pack {

struct Push {
  template <class Prev>
  static constexpr bool accepts_input =
      !std::is_same_v<std::remove_cvref_t<Prev>, void>;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = std::vector<std::remove_cvref_t<Prev>>;

  template <class T>
  constexpr auto operator()(ActionCtx<std::vector<std::remove_cvref_t<T>>> ctx,
                            ActionResult<T> input) const -> ActionResult<void> {
    ctx.arg.get().push_back(std::move(input.value));
    return ActionResult<void>::ok();
  }
};

struct Overwrite {
  template <class Prev>
  static constexpr bool accepts_input =
      !std::is_same_v<std::remove_cvref_t<Prev>, void>;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = std::remove_cvref_t<Prev>;

  template <class T>
  constexpr auto operator()(ActionCtx<std::remove_cvref_t<T>> ctx,
                            ActionResult<T> input) const -> ActionResult<void> {
    ctx.arg.get() = std::move(input.value);
    return ActionResult<void>::ok();
  }
};

struct Optional {
  template <class Prev>
  static constexpr bool accepts_input =
      !std::is_same_v<std::remove_cvref_t<Prev>, void>;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = std::optional<std::remove_cvref_t<Prev>>;

  template <class T>
  constexpr auto operator()(ActionCtx<std::optional<std::remove_cvref_t<T>>> ctx,
                            ActionResult<T> input) const -> ActionResult<void> {
    ctx.arg.get() = std::move(input.value);
    return ActionResult<void>::ok();
  }
};

struct Last {
  template <class Prev>
  static constexpr bool accepts_input =
      !std::is_same_v<std::remove_cvref_t<Prev>, void>;
  template <class Prev>
  using after_type = void;
  template <class Prev>
  using storage_type = std::remove_cvref_t<Prev>;

  template <class T>
  constexpr auto operator()(ActionCtx<std::remove_cvref_t<T>> ctx,
                            ActionResult<T> input) const -> ActionResult<void> {
    ctx.arg.get() = std::move(input.value);
    return ActionResult<void>::ok();
  }
};

struct First {
  template <class Prev>
  static constexpr bool accepts_input =
      !std::is_same_v<std::remove_cvref_t<Prev>, void>;
  template <class Prev>
  using after_type = void;
  template <class Prev>
  using storage_type = std::optional<std::remove_cvref_t<Prev>>;

  template <class T>
  constexpr auto operator()(ActionCtx<std::optional<std::remove_cvref_t<T>>> ctx,
                            ActionResult<T> input) const -> ActionResult<void> {
    if (!ctx.arg.get().has_value()) {
      ctx.arg.get() = std::move(input.value);
    }

    return ActionResult<void>::ok();
  }
};

struct Count {
  template <class Prev>
  static constexpr bool accepts_input =
      !std::is_same_v<std::remove_cvref_t<Prev>, void>;
  template <class Prev>
  using after_type = void;
  template <class Prev>
  using storage_type = std::size_t;

  template <class T>
  constexpr auto operator()([[maybe_unused]] ActionCtx<std::size_t> ctx,
                            ActionResult<T>) const -> ActionResult<void> {
    ++ctx.arg.get();
    return ActionResult<void>::ok();
  }
};

struct Ignore {
  template <class Prev>
  static constexpr bool accepts_input =
      !std::is_same_v<std::remove_cvref_t<Prev>, void>;
  template <class Prev>
  using after_type = void;
  template <class Prev>
  using storage_type = std::monostate;

  template <class T>
  constexpr auto operator()(ActionCtx<std::monostate>, ActionResult<T>) const
      -> ActionResult<void> {
    return ActionResult<void>::ok();
  }
};

static constexpr auto push = Push{};
static constexpr auto overwrite = Overwrite{};
static constexpr auto optional = Optional{};
static constexpr auto last = Last{};
static constexpr auto first = First{};
static constexpr auto count = Count{};
static constexpr auto ignore = Ignore{};

}  // namespace pack

//

};  // namespace argon
