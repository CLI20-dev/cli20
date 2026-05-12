#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include "cli/argument_base.hh"
#include "cli/meta.hh"
#include "cli/relation.hh"
#include "cli/string_literal.hh"

namespace cli::detail {

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

template <class T>
concept HasRelations = requires {
  { T::relations } -> RelationSetLike;
};

template <class Field>
consteval auto relation_target_name() -> std::string_view {
  using F = std::remove_cvref_t<Field>;
  if constexpr (std::derived_from<F, OptionTag> ||
                std::derived_from<F, CommandTag>) {
    return F::name.view();
  } else {
    return {};
  }
}

template <class T>
consteval auto named_relation_targets() {
  return [&]<class... Args>(std::type_identity<std::tuple<Args...>>) consteval
             -> auto {
    return std::array<std::string_view, sizeof...(Args)>{
        relation_target_name<Args>()...};
  }(std::type_identity<
                 std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
}

template <class Array>
consteval auto contains_name(const Array& names, std::string_view name) -> bool {
  for (auto candidate : names) {
    if (!candidate.empty() && candidate == name) return true;
  }
  return false;
}

template <class T, std::size_t Limit, std::size_t I = 0>
consteval auto group_name_exists_before(std::string_view name) -> bool {
  if constexpr (I >= Limit) {
    return false;
  } else {
    constexpr auto rel = std::get<I>(T::relations.items);
    using R = std::remove_cvref_t<decltype(rel)>;
    if constexpr (std::same_as<R, GroupRelation>) {
      if (rel.name == name) return true;
    }
    return group_name_exists_before<T, Limit, I + 1>(name);
  }
}

template <class T, std::size_t I = 0>
consteval auto group_name_exists(std::string_view name) -> bool {
  constexpr auto size =
      std::tuple_size_v<std::remove_cvref_t<decltype(T::relations.items)>>;
  return group_name_exists_before<T, size, I>(name);
}

template <class T>
consteval auto operand_exists(std::string_view name) -> bool {
  constexpr auto field_names = named_relation_targets<T>();
  return contains_name(field_names, name) || group_name_exists<T>(name);
}

template <class T, std::size_t I = 0>
consteval auto relation_membership_count(std::string_view name) -> std::size_t {
  constexpr auto size =
      std::tuple_size_v<std::remove_cvref_t<decltype(T::relations.items)>>;
  if constexpr (I >= size) {
    return 0;
  } else {
    constexpr auto rel = std::get<I>(T::relations.items);
    using R = std::remove_cvref_t<decltype(rel)>;
    std::size_t current = 0;
    if constexpr (std::same_as<R, GroupRelation>) {
      for (auto member : rel.members) {
        if (member == name) ++current;
      }
    } else if constexpr (std::same_as<R, ConflictsRelation>) {
      if (rel.left == name) ++current;
      if (rel.right == name) ++current;
    } else if constexpr (std::same_as<R, DependsOnRelation>) {
      if (rel.source == name) ++current;
      if (rel.target == name) ++current;
    }
    return current + relation_membership_count<T, I + 1>(name);
  }
}

template <class T>
consteval auto field_relation_memberships_are_unique() -> bool {
  constexpr auto field_names = named_relation_targets<T>();
  for (auto name : field_names) {
    if (!name.empty() && relation_membership_count<T>(name) > 1) {
      return false;
    }
  }
  return true;
}

template <class T, std::size_t I = 0>
consteval auto relation_items_are_well_formed() -> bool {
  constexpr auto size =
      std::tuple_size_v<std::remove_cvref_t<decltype(T::relations.items)>>;
  if constexpr (I >= size) {
    return true;
  } else {
    constexpr auto rel = std::get<I>(T::relations.items);
    using R = std::remove_cvref_t<decltype(rel)>;
    constexpr auto field_names = named_relation_targets<T>();
    if constexpr (std::same_as<R, GroupRelation>) {
      if (rel.name.empty() || rel.members.size < 2 ||
          contains_name(field_names, rel.name) ||
          group_name_exists_before<T, I>(rel.name)) {
        return false;
      }
      for (auto member : rel.members) {
        if (!contains_name(field_names, member)) return false;
      }
    } else if constexpr (std::same_as<R, ConflictsRelation>) {
      if (rel.left.empty() || rel.right.empty() || rel.left == rel.right ||
          !operand_exists<T>(rel.left) || !operand_exists<T>(rel.right)) {
        return false;
      }
    } else if constexpr (std::same_as<R, DependsOnRelation>) {
      if (rel.source.empty() || rel.target.empty() || rel.source == rel.target ||
          !operand_exists<T>(rel.source) || !operand_exists<T>(rel.target)) {
        return false;
      }
    } else {
      return false;
    }
    return relation_items_are_well_formed<T, I + 1>();
  }
}

template <class T>
consteval auto relations_are_well_formed() -> bool {
  if constexpr (!HasRelations<T>) {
    return true;
  } else {
    return relation_items_are_well_formed<T>() &&
           field_relation_memberships_are_unique<T>();
  }
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
  if (NargsValue.min < 0) {
    return false;
  }
  if (NargsValue.max != -1 && NargsValue.max < NargsValue.min) {
    return false;
  }
  return true;
}

}  // namespace cli::detail
