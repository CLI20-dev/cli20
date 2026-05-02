#pragma once

#include <algorithm>
#include <array>
#include <string_view>

namespace argon {
template <std::size_t N>
struct StringLiteral {
  std::array<char, N> value{};

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
  consteval StringLiteral(const char (&str)[N]) noexcept { std::copy_n(str, N, value.begin()); }

  [[nodiscard]]
  consteval auto size() const noexcept -> std::size_t {
    return N - 1;
  }

  [[nodiscard]]
  consteval auto data() const noexcept -> const char* {
    return value.data();
  }

  [[nodiscard]]
  consteval auto view() const noexcept -> std::string_view {
    return {value.data(), size()};
  }

  [[nodiscard]]
  consteval auto operator[](std::size_t i) const noexcept -> char {
    return value[i];
  }

  consteval auto operator==(const StringLiteral&) const noexcept -> bool = default;
};

template <std::size_t N>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
StringLiteral(const char (&)[N]) -> StringLiteral<N>;
}  // namespace argon
