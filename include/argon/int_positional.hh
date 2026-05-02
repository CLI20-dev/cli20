#pragma once

#include <argon/argument.hh>
#include <charconv>
#include <expected>
#include <span>
#include <system_error>

namespace argon {

// Full specialization of PositionalArgument for int.
// Provides a parse() method callable by Parser.
template <>
struct PositionalArgument<int> : ArgumentTag {
  static constexpr auto type = ArgumentType::positional;
  using value_type = int;

  [[nodiscard]] constexpr auto value() const noexcept -> const int& { return value_; }
  [[nodiscard]] constexpr auto seen() const noexcept -> bool { return seen_; }

  template <class>
  friend class Parser;

 protected:
  auto parse(std::string_view sv) -> std::expected<void, std::error_code> {
    auto res = std::from_chars(sv.begin(), sv.end(), value_);
    if (res.ec != std::errc() || res.ptr != sv.end()) {
      return std::unexpected(res.ec != std::errc() ? std::make_error_code(res.ec)
                                                   : std::make_error_code(std::errc::invalid_argument));
    }
    return {};
  }

  constexpr auto markSeen() noexcept -> void { seen_ = true; }

 private:
  int value_{};
  bool seen_ = false;
};

using IntPositional = PositionalArgument<int>;

}  // namespace argon
