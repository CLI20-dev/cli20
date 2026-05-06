#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <argon/meta.hh>
#include <argon/string_literal.hh>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "argon/action.hh"

namespace argon {

struct SpecMemberTag {};

struct OptionTag : SpecMemberTag {};
struct PositionalTag : SpecMemberTag {};
struct CommandTag : SpecMemberTag {};
struct DescriptionTag : SpecMemberTag {};

struct Nargs {
  int min = -1;
  int max = -1;
};

enum class Presence { required, optional };

inline constexpr auto required = Presence::required;
inline constexpr auto optional = Presence::optional;

namespace nargs {

inline constexpr Nargs none{.min = 0, .max = 0};
inline constexpr Nargs one{.min = 1, .max = 1};
inline constexpr Nargs zero_or_one{.min = 0, .max = 1};
inline constexpr Nargs zero_or_more{.min = 0, .max = -1};
inline constexpr Nargs one_or_more{.min = 1, .max = -1};

template <int N>
inline constexpr Nargs exactly{.min = N, .max = N};

template <int Min, int Max>
inline constexpr Nargs between{.min = Min, .max = Max};

}  // namespace nargs

namespace detail {

template <class T>
consteval auto members_are_derived_from_valid_argon_class() -> bool {
  return []<class... Args>(std::type_identity<std::tuple<Args...>>) consteval
             -> auto {
    return (std::derived_from<std::remove_cvref_t<Args>, SpecMemberTag> && ...);
  }(std::type_identity<
                 std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
}

template <class T>
consteval auto options_have_unique_long_name() -> bool {
  std::vector<std::string_view> names;
  [&]<class... Args>(std::type_identity<std::tuple<Args...>>) consteval
      -> auto {
    (
        [&]() consteval -> auto {
          if constexpr (std::derived_from<std::remove_cvref_t<Args>,
                                          OptionTag>) {
            names.push_back(std::remove_cvref_t<Args>::name.view());
          }
        }(),
        ...);
  }(std::type_identity<
          std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
  std::ranges::sort(names);
  return std::ranges::adjacent_find(names) == names.end();
}

template <class T>
consteval auto options_have_unique_short_name() -> bool {
  std::vector<char> names;
  [&]<class... Args>(std::type_identity<std::tuple<Args...>>) consteval
      -> auto {
    (
        [&]() consteval -> auto {
          if constexpr (std::derived_from<std::remove_cvref_t<Args>,
                                          OptionTag>) {
            if (std::remove_cvref_t<Args>::short_name != '\0') {
              names.push_back(std::remove_cvref_t<Args>::short_name);
            }
          }
        }(),
        ...);
  }(std::type_identity<
          std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
  std::ranges::sort(names);
  return std::ranges::adjacent_find(names) == names.end();
}

template <class T>
consteval auto commands_have_unique_long_name() -> bool {
  std::vector<std::string_view> names;
  [&]<class... Args>(std::type_identity<std::tuple<Args...>>) consteval
      -> auto {
    (
        [&]() consteval -> auto {
          if constexpr (std::derived_from<std::remove_cvref_t<Args>,
                                          CommandTag>) {
            names.push_back(std::remove_cvref_t<Args>::name.view());
          }
        }(),
        ...);
  }(std::type_identity<
          std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
  std::ranges::sort(names);
  return std::ranges::adjacent_find(names) == names.end();
}

template <class T>
consteval auto positionals_have_variadic_at_end() {
  bool found_variadic = false;
  bool found_positional_after_variadic = false;
  [&]<class... Args>(std::type_identity<std::tuple<Args...>>) consteval
      -> auto {
    (
        [&]() consteval -> auto {
          if constexpr (std::derived_from<std::remove_cvref_t<Args>,
                                          PositionalTag>) {
            if (found_variadic) {
              found_positional_after_variadic = true;
            }
            if (std::remove_cvref_t<Args>::nargs.max !=
                std::remove_cvref_t<Args>::nargs.min) {
              found_variadic = true;
            }
          }
        }(),
        ...);
  }(std::type_identity<
          std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
  return !found_positional_after_variadic;
}

template <StringLiteral Name>
[[nodiscard]]
constexpr auto is_valid_long_option_name() noexcept -> bool {
  constexpr auto size = Name.size();

  if constexpr (size == 0) {
    return false;
  }
  auto is_alpha = [](char c) consteval -> bool {
    return ('a' <= c && c <= 'z');
  };
  auto is_digit = [](char c) consteval -> bool { return '0' <= c && c <= '9'; };
  auto is_alnum = [&](char c) consteval -> bool {
    return is_alpha(c) || is_digit(c);
  };

  if (!is_alpha(Name[0])) {
    return false;
  }
  bool previous_is_hyphen = false;
  for (std::size_t i = 1; i < size; ++i) {
    const char c = Name[i];
    if (c == '-') {
      if (previous_is_hyphen) {
        return false;
      }
      previous_is_hyphen = true;
      continue;
    }
    if (!is_alnum(c)) {
      return false;
    }
    previous_is_hyphen = false;
  }

  return !previous_is_hyphen;
}

constexpr auto is_valid_short_option_name(char Name) noexcept -> bool {
  return ('a' <= Name && Name <= 'z') || ('A' <= Name && Name <= 'Z') ||
         Name == '\0';
}

template <StringLiteral Name>
[[nodiscard]]
consteval auto is_valid_command_name() noexcept {
  return is_valid_long_option_name<Name>();
}

template <Nargs nargs>
[[nodiscard]]
consteval auto is_valid_nargs() noexcept -> bool {
  if (nargs.max == -1 && nargs.min == -1) {
    return false;
  }
  return true;
}

}  // namespace detail

template <class T>
concept ArgumentSpec = requires {
  requires detail::members_are_derived_from_valid_argon_class<T>();
  requires detail::options_have_unique_long_name<T>();
  requires detail::options_have_unique_short_name<T>();
  requires detail::commands_have_unique_long_name<T>();
  requires detail::positionals_have_variadic_at_end<T>();
};

template <class T>
struct ArgParameter {
  std::string_view help{};
  Presence presence{Presence::optional};
  T default_value{};
};

template <StringLiteral Name, char ShortName, Nargs N, Action A>
  requires requires {
    detail::is_valid_long_option_name<Name>();
    detail::is_valid_short_option_name(ShortName);
    0 <= N.min;
    -1 <= N.max;
    (N.max == -1 || N.min <= N.max);
  }
struct ArgImpl : public OptionTag {
  using value_type =
      typename std::remove_cvref_t<decltype(A.validate().second)>::type;
  ArgImpl() = default;
  ArgImpl(ArgParameter<value_type> param)
      : help(param.help),
        presence(param.presence),
        value_(param.default_value) {}
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  std::string_view help{};
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  argon::Presence presence{Presence::optional};
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  value_type default_value{};

  static constexpr auto name = Name;
  static constexpr auto short_name = ShortName;
  static constexpr auto nargs = N;

  // Called once when the option token is seen (before processing its values).
  auto notify_option_seen() -> void { ++option_occurrences_; }

  // Called once per value token associated with this option.
  auto invoke_action(std::string_view text, std::size_t arg_index)
      -> ActionResult<void> {
    provided_ = true;
    ActionCtx<value_type> ctx{
        .index = arg_index,
        .occurrences = option_occurrences_,
        .invoke_count = invoke_count_,
        .arg = std::ref(value_),
    };
    ++invoke_count_;
    auto result =
        decltype(A)::invoke(ctx, ActionResult<std::string_view>::ok(text));
    if (result.has_error()) {
      return ActionResult<void>::fail(std::move(result.error));
    }
    return ActionResult<void>::ok();
  }

  // Called once for flags (nargs = {0,0}) after the option token is seen.
  auto invoke_flag(std::size_t arg_index) -> ActionResult<void> {
    provided_ = true;
    ActionCtx<value_type> ctx{
        .index = arg_index,
        .occurrences = option_occurrences_,
        .invoke_count = invoke_count_,
        .arg = std::ref(value_),
    };
    ++invoke_count_;
    auto result = decltype(A)::invoke(
        ctx, ActionResult<std::string_view>::ok(std::string_view{}));
    if (result.has_error()) {
      return ActionResult<void>::fail(std::move(result.error));
    }
    return ActionResult<void>::ok();
  }

  [[nodiscard]] auto value() const -> const value_type& { return value_; }
  [[nodiscard]] auto value() -> value_type& { return value_; }
  [[nodiscard]] auto provided() const -> bool { return provided_; }

 private:
  template <ArgumentSpec>
  friend struct Parser;

  value_type value_{};
  std::size_t option_occurrences_{};  // times the option token appeared
  std::size_t invoke_count_{};        // times invoke_action/invoke_flag called
  bool provided_{};
};

template <Nargs N, Action A>
  requires requires {
    0 <= N.min;
    -1 <= N.max;
    (N.max == -1 || N.min <= N.max);
  }
struct PositionalImpl : public PositionalTag {
  using value_type =
      typename std::remove_cvref_t<decltype(A.validate().second)>::type;

  PositionalImpl() = default;
  PositionalImpl(ArgParameter<value_type> param)
      : help(param.help),
        presence(param.presence),
        value_(param.default_value) {}
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  std::string_view help{};
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  argon::Presence presence{Presence::optional};
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  value_type default_value{};

  static constexpr auto nargs = N;

  // Called once per value token for this positional.
  auto invoke_action(std::string_view text, std::size_t arg_index)
      -> ActionResult<void> {
    provided_ = true;
    ActionCtx<value_type> ctx{
        .index = arg_index,
        .occurrences = occurrence_count_,
        .invoke_count = invoke_count_,
        .arg = std::ref(value_),
    };
    ++occurrence_count_;
    ++invoke_count_;
    auto result =
        decltype(A)::invoke(ctx, ActionResult<std::string_view>::ok(text));
    if (result.has_error()) {
      return ActionResult<void>::fail(std::move(result.error));
    }
    return ActionResult<void>::ok();
  }

  [[nodiscard]] auto value() const -> const value_type& { return value_; }
  [[nodiscard]] auto value() -> value_type& { return value_; }
  [[nodiscard]] auto provided() const -> bool { return provided_; }

 private:
  template <ArgumentSpec>
  friend struct Parser;

  value_type value_{};
  std::size_t occurrence_count_{};  // times a value was provided
  std::size_t
      invoke_count_{};  // times invoke_action was called (same for positionals)
  bool provided_{};
};

struct Description : public std::string, DescriptionTag {};

struct CommandParameter {
  std::string_view help{};
};

template <StringLiteral Name, ArgumentSpec T>
  requires requires { detail::is_valid_command_name<Name>(); }
struct Command : public T, public CommandTag {
  using T::T;
  using argument_type = T;

  Command() = default;
  Command(CommandParameter param) : help(param.help) {}
  static constexpr auto name = Name;
  static constexpr auto commandName() -> std::string_view { return Name.view(); }
  [[nodiscard]] auto provided() const -> bool { return provided_; }
  [[nodiscard]] auto helpText() const -> std::string_view { return help; }
  auto mark_provided() -> void { provided_ = true; }

 private:
  std::string_view help{};
  bool provided_{};
};

namespace detail {

template <class T>
struct ActionFor;

template <>
struct ActionFor<std::string> {
  inline static constexpr auto set_once =
      Action<conversion::string, pack::set_once>{};
  inline static constexpr auto push =
      Action<conversion::string, pack::push>{};
};

template <>
struct ActionFor<bool> {
  inline static constexpr auto set_once =
      Action<conversion::boolean, pack::set_once>{};
  inline static constexpr auto push =
      Action<conversion::boolean, pack::push>{};
};

template <>
struct ActionFor<std::filesystem::path> {
  inline static constexpr auto set_once =
      Action<conversion::path, pack::set_once>{};
  inline static constexpr auto push =
      Action<conversion::path, pack::push>{};
};

template <std::integral T>
struct ActionFor<T> {
  inline static constexpr auto set_once =
      Action<conversion::integer<T>, pack::set_once>{};
  inline static constexpr auto push =
      Action<conversion::integer<T>, pack::push>{};
};

template <std::floating_point T>
struct ActionFor<T> {
  inline static constexpr auto set_once =
      Action<conversion::floating<T>, pack::set_once>{};
  inline static constexpr auto push =
      Action<conversion::floating<T>, pack::push>{};
};

template <class T, Nargs N>
struct PositionalActionFor {
  inline static constexpr auto value =
      [] {
        if constexpr (N.max == 1) {
          return ActionFor<T>::set_once;
        } else {
          return ActionFor<T>::push;
        }
      }();
};

}  // namespace detail

template <StringLiteral Name, char ShortName = '\0'>
using FlagOption = ArgImpl<Name, ShortName, nargs::none,
                           Action<pack::set_true>{}>;

template <StringLiteral Name, char ShortName = '\0'>
using FlagArg = FlagOption<Name, ShortName>;

template <StringLiteral Name = "help", char ShortName = 'h'>
using HelpFlag =
    ArgImpl<Name, ShortName, nargs::none,
            Action<action::print_help, action::exit_success>{}>;

template <class T, StringLiteral Name, char ShortName = '\0'>
using ScalarOption =
    ArgImpl<Name, ShortName, nargs::one, detail::ActionFor<T>::set_once>;

template <class T, StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using ListOption = ArgImpl<Name, ShortName, N,
                           detail::ActionFor<T>::push>;

template <class T, Nargs N = nargs::one>
using Positional =
    PositionalImpl<N, detail::PositionalActionFor<T, N>::value>;

template <StringLiteral Name, char ShortName = '\0'>
using StringOption = ScalarOption<std::string, Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0'>
using StringArg = StringOption<Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using StringListOption = ListOption<std::string, Name, ShortName, N>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using StringListArg = StringListOption<Name, ShortName, N>;
using StringPositional = Positional<std::string>;
using StringPositionalArg = StringPositional;

template <StringLiteral Name, char ShortName = '\0'>
using StrOption = StringOption<Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0'>
using StrArg = StrOption<Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using StrListOption = StringListOption<Name, ShortName, N>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using StrListArg = StrListOption<Name, ShortName, N>;
using StrPositional = StringPositional;
using StrPositionalArg = StrPositional;
using StrPosArg = StrPositional;

template <StringLiteral Name, char ShortName = '\0'>
using BoolOption = ScalarOption<bool, Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0'>
using BoolArg = BoolOption<Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using BoolListOption = ListOption<bool, Name, ShortName, N>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using BoolListArg = BoolListOption<Name, ShortName, N>;
using BoolPositional = Positional<bool>;
using BoolPositionalArg = BoolPositional;
using BoolPosArg = BoolPositional;

template <StringLiteral Name, char ShortName = '\0'>
using IntOption = ScalarOption<int, Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0'>
using IntArg = IntOption<Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using IntListOption = ListOption<int, Name, ShortName, N>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using IntListArg = IntListOption<Name, ShortName, N>;
using IntPositional = Positional<int>;
using IntPositionalArg = IntPositional;
using IntPosArg = IntPositional;

template <StringLiteral Name, char ShortName = '\0'>
using Int32Option = ScalarOption<std::int32_t, Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0'>
using Int32Arg = Int32Option<Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using Int32ListOption = ListOption<std::int32_t, Name, ShortName, N>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using Int32ListArg = Int32ListOption<Name, ShortName, N>;
using Int32Positional = Positional<std::int32_t>;
using Int32PositionalArg = Int32Positional;
using Int32PosArg = Int32Positional;

template <StringLiteral Name, char ShortName = '\0'>
using Int64Option = ScalarOption<std::int64_t, Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0'>
using Int64Arg = Int64Option<Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using Int64ListOption = ListOption<std::int64_t, Name, ShortName, N>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using Int64ListArg = Int64ListOption<Name, ShortName, N>;
using Int64Positional = Positional<std::int64_t>;
using Int64PositionalArg = Int64Positional;
using Int64PosArg = Int64Positional;

template <StringLiteral Name, char ShortName = '\0'>
using Uint32Option = ScalarOption<std::uint32_t, Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0'>
using Uint32Arg = Uint32Option<Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using Uint32ListOption = ListOption<std::uint32_t, Name, ShortName, N>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using Uint32ListArg = Uint32ListOption<Name, ShortName, N>;
using Uint32Positional = Positional<std::uint32_t>;
using Uint32PositionalArg = Uint32Positional;
using Uint32PosArg = Uint32Positional;

template <StringLiteral Name, char ShortName = '\0'>
using Uint64Option = ScalarOption<std::uint64_t, Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0'>
using Uint64Arg = Uint64Option<Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using Uint64ListOption = ListOption<std::uint64_t, Name, ShortName, N>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using Uint64ListArg = Uint64ListOption<Name, ShortName, N>;
using Uint64Positional = Positional<std::uint64_t>;
using Uint64PositionalArg = Uint64Positional;
using Uint64PosArg = Uint64Positional;

template <StringLiteral Name, char ShortName = '\0'>
using FloatOption = ScalarOption<float, Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0'>
using FloatArg = FloatOption<Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using FloatListOption = ListOption<float, Name, ShortName, N>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using FloatListArg = FloatListOption<Name, ShortName, N>;
using FloatPositional = Positional<float>;
using FloatPositionalArg = FloatPositional;
using FloatPosArg = FloatPositional;

template <StringLiteral Name, char ShortName = '\0'>
using DoubleOption = ScalarOption<double, Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0'>
using DoubleArg = DoubleOption<Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using DoubleListOption = ListOption<double, Name, ShortName, N>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using DoubleListArg = DoubleListOption<Name, ShortName, N>;
using DoublePositional = Positional<double>;
using DoublePositionalArg = DoublePositional;
using DoublePosArg = DoublePositional;

template <StringLiteral Name, char ShortName = '\0'>
using PathOption = ScalarOption<std::filesystem::path, Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0'>
using PathArg = PathOption<Name, ShortName>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using PathListOption = ListOption<std::filesystem::path, Name, ShortName, N>;
template <StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using PathListArg = PathListOption<Name, ShortName, N>;
using PathPositional = Positional<std::filesystem::path>;
using PathPositionalArg = PathPositional;
using PathPosArg = PathPositional;

}  // namespace argon
