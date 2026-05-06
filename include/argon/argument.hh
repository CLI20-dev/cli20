#pragma once

#include <algorithm>
#include <argon/meta.hh>
#include <argon/string_literal.hh>
#include <optional>
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
  Presence presence{};
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
  ArgImpl(ArgParameter<value_type> param)
      : help(param.help),
        presence(param.presence),
        value_(param.default_value) {}
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  std::string_view help;
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  argon::Presence presence;
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  value_type default_value;

  static constexpr auto name = Name;
  static constexpr auto short_name = ShortName;
  static constexpr auto nargs = N;

 private:
  value_type value_{};
  int occurrences_{};
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

  PositionalImpl(ArgParameter<value_type> param)
      : help(param.help),
        presence(param.presence),
        value_(param.default_value) {}
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  std::string_view help;
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  argon::Presence presence;
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  value_type default_value;

  static constexpr auto nargs = N;

 private:
  value_type value_{};
  int occurrences_{};
};

struct Description : public std::string, DescriptionTag {};

struct CommandParameter {
  std::string_view help{};
};

template <StringLiteral Name, ArgumentSpec T>
  requires requires { detail::is_valid_command_name<Name>(); }
struct Command : public T, public CommandTag {
  using T::T;

  Command(CommandParameter param) : help(param.help) {}
  static constexpr auto name = Name;

 private:
  std::string_view help{};
};

}  // namespace argon
