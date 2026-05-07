#pragma once

#include <algorithm>
#include <cli/meta.hh>
#include <cli/string_literal.hh>
#include <filesystem>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

#include "cli/action.hh"

namespace cli {

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
consteval auto members_are_derived_from_valid_cli_class() -> bool {
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
            if constexpr (std::remove_cvref_t<Args>::short_name != '\0') {
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
  auto is_alpha = [](char c) constexpr -> bool {
    return ('a' <= c && c <= 'z');
  };
  auto is_digit = [](char c) constexpr -> bool { return '0' <= c && c <= '9'; };
  auto is_alnum = [&](char c) constexpr -> bool {
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

template <Nargs NargsValue>
[[nodiscard]]
consteval auto is_valid_nargs() noexcept -> bool {
  if (NargsValue.max == -1 && NargsValue.min == -1) {
    return false;
  }
  return true;
}

}  // namespace detail

template <class T>
concept ArgumentSpec = requires {
  requires detail::members_are_derived_from_valid_cli_class<T>();
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
  std::function<void(const T&)> on_parse{};
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
        value_(param.default_value),
        on_parse_(std::move(param.on_parse)) {}

  // Convenience constructor for StoreInto variants (value_type == T*).
  explicit ArgImpl(std::remove_pointer_t<value_type>& ref)
    requires std::is_pointer_v<value_type>
      : value_(&ref) {}
  ArgImpl(std::remove_pointer_t<value_type>& ref, std::string_view help_text)
    requires std::is_pointer_v<value_type>
      : help(help_text), value_(&ref) {}

  // Bind to an external variable after construction.
  // Allows: opt.bind(var)  or  opt = &var
  auto bind(std::remove_pointer_t<value_type>& ref) -> ArgImpl&
    requires std::is_pointer_v<value_type>
  {
    value_ = &ref;
    return *this;
  }
  auto operator=(std::remove_pointer_t<value_type>* ptr) -> ArgImpl&
    requires std::is_pointer_v<value_type>
  {
    value_ = ptr;
    return *this;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  std::string_view help{};
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  cli::Presence presence{Presence::optional};
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  value_type default_value{};

  static constexpr auto name = Name;
  static constexpr auto short_name = ShortName;
  static constexpr auto nargs = N;

  // Called once when the option token is seen (before processing its values).
  auto notify_option_seen() -> void { ++occurrence_count_; }

  // Called once per value token associated with this option.
  auto invoke_action(std::string_view text, std::size_t arg_index)
      -> ActionResult<void> {
    provided_ = true;
    ActionCtx<value_type> ctx{
        .index = arg_index,
        .occurrences = occurrence_count_,
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
        .occurrences = occurrence_count_,
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

  // Called once after all value tokens for one option occurrence are processed.
  auto fire_on_parse() -> void {
    if (on_parse_) on_parse_(value_);
  }

  [[nodiscard]] auto value() const -> const value_type& { return value_; }
  [[nodiscard]] auto value() -> value_type& { return value_; }
  [[nodiscard]] auto provided() const -> bool { return provided_; }

 private:
  template <ArgumentSpec>
  friend struct Parser;

  value_type value_{};
  std::function<void(const value_type&)> on_parse_{};
  std::size_t occurrence_count_{};  // times the option token appeared
  std::size_t invoke_count_{};      // times invoke_action/invoke_flag called
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
        value_(param.default_value),
        on_parse_(std::move(param.on_parse)) {}
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  std::string_view help{};
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  cli::Presence presence{Presence::optional};
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

  // Called once after all tokens for this positional are consumed.
  auto fire_on_parse() -> void {
    if (on_parse_) on_parse_(value_);
  }

  [[nodiscard]] auto value() const -> const value_type& { return value_; }
  [[nodiscard]] auto value() -> value_type& { return value_; }
  [[nodiscard]] auto provided() const -> bool { return provided_; }

 private:
  template <ArgumentSpec>
  friend struct Parser;

  value_type value_{};
  std::function<void(const value_type&)> on_parse_{};
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
  static constexpr auto command_name() -> std::string_view {
    return Name.view();
  }
  [[nodiscard]] auto provided() const -> bool { return provided_; }
  [[nodiscard]] auto help_text() const -> std::string_view { return help; }
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
  inline static constexpr auto set_once = conversion::string | pack::set_once;
  inline static constexpr auto push = conversion::string | pack::push;
  inline static constexpr auto store_into =
      conversion::string | pack::store_into<std::string>;
};

template <>
struct ActionFor<bool> {
  inline static constexpr auto set_once = conversion::boolean | pack::set_once;
  inline static constexpr auto push = conversion::boolean | pack::push;
  inline static constexpr auto store_into =
      conversion::boolean | pack::store_into<bool>;
};

template <>
struct ActionFor<std::filesystem::path> {
  inline static constexpr auto set_once = conversion::path | pack::set_once;
  inline static constexpr auto push = conversion::path | pack::push;
  inline static constexpr auto store_into =
      conversion::path | pack::store_into<std::filesystem::path>;
};

template <std::integral T>
struct ActionFor<T> {
  inline static constexpr auto set_once =
      conversion::integer<T> | pack::set_once;
  inline static constexpr auto push = conversion::integer<T> | pack::push;
  inline static constexpr auto store_into =
      conversion::integer<T> | pack::store_into<T>;
};

template <std::floating_point T>
struct ActionFor<T> {
  inline static constexpr auto set_once =
      conversion::floating<T> | pack::set_once;
  inline static constexpr auto push = conversion::floating<T> | pack::push;
  inline static constexpr auto store_into =
      conversion::floating<T> | pack::store_into<T>;
};

template <class T, Nargs N>
struct PositionalActionFor {
  inline static constexpr auto value = []() -> auto {
    if constexpr (N.max == 1) {
      return ActionFor<T>::set_once;
    } else {
      return ActionFor<T>::push;
    }
  }();
};

}  // namespace detail

// ── Core public API ───────────────────────────────────────────────────────────

template <StringLiteral Name, char ShortName = '\0'>
using Flag = ArgImpl<Name, ShortName, nargs::none, pack::set_true>;

template <class T, StringLiteral Name, char ShortName = '\0'>
using BoundOption =
    ArgImpl<Name, ShortName, nargs::one, detail::ActionFor<T>::store_into>;

template <StringLiteral Name, char ShortName = '\0'>
using BoundStringOption = BoundOption<std::string, Name, ShortName>;

template <StringLiteral Name, char ShortName = '\0'>
using BoundIntOption = BoundOption<int, Name, ShortName>;

template <StringLiteral Name, char ShortName = '\0'>
using BoundDoubleOption = BoundOption<double, Name, ShortName>;

template <StringLiteral Name, char ShortName = '\0'>
using BoundPathOption = BoundOption<std::filesystem::path, Name, ShortName>;

template <StringLiteral Name = "help", char ShortName = 'h'>
using Help = ArgImpl<Name, ShortName, nargs::none,
                     action::print_help | action::exit_success>;

template <class T, StringLiteral Name, char ShortName = '\0'>
using Option =
    ArgImpl<Name, ShortName, nargs::one, detail::ActionFor<T>::set_once>;

template <class T, StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using ListOption = ArgImpl<Name, ShortName, N, detail::ActionFor<T>::push>;

template <class T, Nargs N = nargs::one>
using Positional = PositionalImpl<N, detail::PositionalActionFor<T, N>::value>;

// ── Convenience aliases for common types ─────────────────────────────────────

template <StringLiteral Name, char ShortName = '\0'>
using StringOption = Option<std::string, Name, ShortName>;

template <StringLiteral Name, char ShortName = '\0'>
using IntOption = Option<int, Name, ShortName>;

template <StringLiteral Name, char ShortName = '\0'>
using DoubleOption = Option<double, Name, ShortName>;

template <StringLiteral Name, char ShortName = '\0'>
using PathOption = Option<std::filesystem::path, Name, ShortName>;

}  // namespace cli
