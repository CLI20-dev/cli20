#pragma once

#include <algorithm>
#include <argon/meta.hh>
#include <argon/string_literal.hh>
#include <optional>
#include <vector>

namespace argon {

struct SpecMemberTag {};

struct OptionTag : SpecMemberTag {};
struct PositionalTag : SpecMemberTag {};
struct CommandTag : SpecMemberTag {};
struct DescriptionTag : SpecMemberTag {};

namespace detail {

template <class T>
consteval auto members_are_derived_from_valid_argon_class() -> bool {
  return []<class... Args>(std::type_identity<std::tuple<Args...>>) consteval -> auto {
    return (std::derived_from<std::remove_cvref_t<Args>, SpecMemberTag> && ...);
  }(std::type_identity<std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
}

template <class T>
consteval auto options_have_unique_long_name() -> bool {
  std::vector<std::string_view> names;
  [&]<class... Args>(std::type_identity<std::tuple<Args...>>) consteval -> auto {
    (
        [&]() consteval -> auto {
          if constexpr (std::derived_from<std::remove_cvref_t<Args>, OptionTag>) {
            names.push_back(std::remove_cvref_t<Args>::name.view());
          }
        }(),
        ...);
  }(std::type_identity<std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
  std::ranges::sort(names);
  return std::ranges::adjacent_find(names) == names.end();
}

template <class T>
consteval auto options_have_unique_short_name() -> bool {
  std::vector<char> names;
  [&]<class... Args>(std::type_identity<std::tuple<Args...>>) consteval -> auto {
    (
        [&]() consteval -> auto {
          if constexpr (std::derived_from<std::remove_cvref_t<Args>, OptionTag>) {
            if (std::remove_cvref_t<Args>::short_name != '\0') {
              names.push_back(std::remove_cvref_t<Args>::short_name);
            }
          }
        }(),
        ...);
  }(std::type_identity<std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
  std::ranges::sort(names);
  return std::ranges::adjacent_find(names) == names.end();
}

template <class T>
consteval auto commands_have_unique_long_name() -> bool {
  std::vector<std::string_view> names;
  [&]<class... Args>(std::type_identity<std::tuple<Args...>>) consteval -> auto {
    (
        [&]() consteval -> auto {
          if constexpr (std::derived_from<std::remove_cvref_t<Args>, CommandTag>) {
            names.push_back(std::remove_cvref_t<Args>::name.view());
          }
        }(),
        ...);
  }(std::type_identity<std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
  std::ranges::sort(names);
  return std::ranges::adjacent_find(names) == names.end();
}

template <class T>
consteval auto positionals_have_variadic_at_end() {
  bool found_variadic = false;
  bool found_positional_after_variadic = false;
  [&]<class... Args>(std::type_identity<std::tuple<Args...>>) consteval -> auto {
    (
        [&]() consteval -> auto {
          if constexpr (std::derived_from<std::remove_cvref_t<Args>, PositionalTag>) {
            if (found_variadic) {
              found_positional_after_variadic = true;
            }
            if (std::remove_cvref_t<Args>::nargs.max != std::remove_cvref_t<Args>::nargs.min) {
              found_variadic = true;
            }
          }
        }(),
        ...);
  }(std::type_identity<std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
  return !found_positional_after_variadic;
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

struct Nargs {
  int min = -1;
  int max = -1;
};

enum class Presence { required, optional };

template <class T>
struct ActionResult {
  bool success{};
  size_t index{};
  T value{};
  using value_type = T;
};

template <auto FnHead, auto... FnTail>
  requires std::invocable<decltype(FnHead), ActionResult<std::string_view>>
struct Action {
  template <auto... Fns>
  static constexpr auto then(Action<Fns...>) {
    return Action<FnHead, Fns...>{};
  }

  template <class T>
  static inline auto invoke(ActionResult<T> input) {
    if constexpr (sizeof...(FnTail) == 0) {
      return FnHead(input);
    } else {
      return Action<FnTail...>::invoke(FnHead(input));
    }
  };
};

constexpr auto action = Action<[](auto input) -> auto { return input; }>{};
constexpr auto always_true = Action<[](auto) -> ActionResult<bool> {
  return {.success = true, .index = 0, .value = true};
}>{};
constexpr auto always_string = Action<[](auto) -> ActionResult<std::string> {
  return {.success = true, .index = 0, .value = "hello"};
}>{};

template <auto Fn, auto... Fns>
constexpr auto operator|(Action<Fns...> lhs, Action<Fn>) {
  return lhs;
}

template <class T>
struct ArgParameter {
  std::string_view help{};
  Presence presence{};
  T default_value{};
};

template <StringLiteral Name, char ShortName, Nargs nargs, Action action>
struct ArgImpl : public OptionTag {
  using action_result_type = decltype(action.invoke(ActionResult<std::string_view>{}))::value_type;
  using value_type = std::conditional_t<
      nargs.max == 0, bool,
      std::conditional_t<nargs.min == 0 && nargs.max == 1, std::optional<action_result_type>,
                         std::conditional_t<nargs.min == 1 && nargs.max == 1, action_result_type,
                                            std::vector<action_result_type>>>>;
  ArgImpl(ArgParameter<value_type> param)
      : help(param.help), presence(param.presence), value_(param.default_value) {}
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  std::string_view help;
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  argon::Presence presence;
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  value_type default_value;

  static constexpr auto name = Name;
  static constexpr auto short_name = ShortName;

 private:
  value_type value_{};
  int occurrences_{};
};

template <Nargs Nargs, Action action>
struct PositionalImpl : public PositionalTag {
  using action_result_type = decltype(action.invoke(ActionResult<std::string_view>{}))::value_type;
  using value_type = std::conditional_t<
      Nargs.max == 0, bool,
      std::conditional_t<Nargs.min == 0 && Nargs.max == 1, std::optional<action_result_type>,
                         std::conditional_t<Nargs.min == 1 && Nargs.max == 1, action_result_type,
                                            std::vector<action_result_type>>>>;
  PositionalImpl(ArgParameter<value_type> param)
      : help(param.help), presence(param.presence), value_(param.default_value) {}
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  std::string_view help;
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  argon::Presence presence;
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  value_type default_value;

  static constexpr auto nargs = Nargs;

 private:
  value_type value_{};
  int occurrences_{};
};

struct Description : public std::string, DescriptionTag {};

struct CommandParameter {
  std::string_view help{};
};

template <StringLiteral Name, ArgumentSpec T>
struct Command : public T, public CommandTag {
  using T::T;

  Command(CommandParameter param) : T(param) {}

  static constexpr auto name = Name;

 private:
  std::string_view help{};
};

}  // namespace argon
