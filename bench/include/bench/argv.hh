#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace bench {

struct Argv {
  std::vector<std::string> storage;
  std::vector<char*> raw;
  std::vector<std::string_view> views;

  Argv(std::initializer_list<std::string_view> args) {
    storage.reserve(args.size());
    raw.reserve(args.size());
    views.reserve(args.size());
    for (std::string_view arg : args) {
      storage.emplace_back(arg);
    }
    refresh();
  }

  auto refresh() -> void {
    raw.clear();
    views.clear();
    for (auto& arg : storage) {
      raw.push_back(arg.data());
      views.emplace_back(arg);
    }
  }

  [[nodiscard]] auto argc() const -> int { return static_cast<int>(raw.size()); }

  [[nodiscard]] auto argv() -> char** { return raw.data(); }

  [[nodiscard]] auto span() const -> std::span<const std::string_view> {
    return views;
  }
};

}  // namespace bench
