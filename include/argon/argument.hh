#pragma once

#include <argon/string_literal.hh>
#include <charconv>
#include <concepts>
#include <expected>
#include <string>
#include <system_error>
#include <vector>

namespace argon {

namespace detail {

template <StringLiteral Name>
[[nodiscard]]
consteval auto IsLongOptionName() noexcept -> bool {
  constexpr auto size = Name.size();

  if constexpr (size == 0) {
    return false;
  }
  auto is_alpha = [&](char c) consteval -> bool { return ('a' <= c && c <= 'z'); };
  auto is_digit = [](char c) consteval -> bool { return '0' <= c && c <= '9'; };
  auto is_alnum = [&](char c) consteval -> bool { return is_alpha(c) || is_digit(c); };

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

consteval auto IsShortOptionName(char Name) noexcept -> bool {
  return ('a' <= Name && Name <= 'z') || ('A' <= Name && Name <= 'Z');
}

}  // namespace detail

struct ArgumentTag {};

template <std::derived_from<ArgumentTag>>
class Parser;

template <class ValueT, StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsShortOptionName(ShortOpt) && detail::IsLongOptionName<LongOpt>())
struct ArgBase : ArgumentTag {
  ValueT value_ = {};
  ArgBase(const ArgBase&) = delete;
  auto operator=(const ArgBase&) -> ArgBase& = delete;
  ArgBase(ArgBase&&) = default;
  auto operator=(ArgBase&&) -> ArgBase& = default;
  ArgBase() = default;
  virtual ~ArgBase() = default;

 protected:
  virtual auto parse(std::vector<std::string_view> sv) -> std::expected<void, std::error_code> = 0;
  virtual auto validate() -> std::expected<void, std::error_code> { return {}; }

  template <std::derived_from<ArgumentTag>>
  friend class Parser;
};

template <class ValueT, StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsLongOptionName<LongOpt>() && detail::IsShortOptionName(ShortOpt))
struct Arg : public ArgBase<ValueT, LongOpt, ShortOpt> {
  using ArgBase<ValueT, LongOpt, ShortOpt>::ArgBase;
};

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsLongOptionName<LongOpt>() && detail::IsShortOptionName(ShortOpt))
struct Arg<int, LongOpt, ShortOpt> : public ArgBase<int, LongOpt, ShortOpt> {
  using ArgBase<int, LongOpt, ShortOpt>::ArgBase;

 protected:
  auto parse(std::vector<std::string_view> sv) -> std::expected<void, std::error_code> override {
    auto res = std::from_chars(sv[0].begin(), sv[0].end(), this->value_);
    if (res.ec != std::errc()) {
      return std::unexpected(std::make_error_code(res.ec));
    }
    return {};
  }

  auto validate() -> std::expected<void, std::error_code> override { return {}; }
};

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsLongOptionName<LongOpt>() && detail::IsShortOptionName(ShortOpt))
using IntArg = Arg<int, LongOpt, ShortOpt>;

}  // namespace argon
