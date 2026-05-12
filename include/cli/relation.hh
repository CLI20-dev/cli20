#pragma once

#include <array>
#include <initializer_list>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace cli {

template <std::size_t Max = 8>
struct NameList {
  std::array<std::string_view, Max> values{};
  std::size_t size{};

  constexpr NameList() = default;
  constexpr NameList(std::initializer_list<std::string_view> names) {
    if (names.size() > Max) {
      throw "cli::NameList capacity exceeded";
    }
    size = names.size();
    auto it = names.begin();
    for (std::size_t i = 0; i < size; ++i, ++it) {
      values[i] = *it;
    }
  }

  [[nodiscard]] constexpr auto begin() const { return values.begin(); }
  [[nodiscard]] constexpr auto end() const { return values.begin() + size; }
  [[nodiscard]] constexpr auto empty() const -> bool { return size == 0; }
};

struct GroupParams {
  std::string_view name;
  NameList<> members;
};

struct GroupRelation {
  std::string_view name;
  NameList<> members;
};

struct ConflictsRelation {
  std::string_view left;
  std::string_view right;
};

struct DependsOnRelation {
  std::string_view source;
  std::string_view target;
};

template <class... Relations>
struct RelationSet {
  std::tuple<Relations...> items;
};

template <class T>
struct IsRelationSet : std::false_type {};

template <class... Relations>
struct IsRelationSet<RelationSet<Relations...>> : std::true_type {};

template <class T>
concept RelationSetLike = IsRelationSet<std::remove_cvref_t<T>>::value;

[[nodiscard]] constexpr auto group(GroupParams params) -> GroupRelation {
  return {.name = params.name, .members = params.members};
}

[[nodiscard]] constexpr auto conflicts(std::string_view left,
                                       std::string_view right)
    -> ConflictsRelation {
  return {.left = left, .right = right};
}

[[nodiscard]] constexpr auto depends_on(std::string_view source,
                                        std::string_view target)
    -> DependsOnRelation {
  return {.source = source, .target = target};
}

template <class... Relations>
[[nodiscard]] constexpr auto relations(Relations... rels)
    -> RelationSet<Relations...> {
  return {.items = std::tuple<Relations...>{rels...}};
}

}  // namespace cli
