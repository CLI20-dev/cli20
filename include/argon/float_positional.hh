#pragma once

#include <argon/argument.hh>
#include <argon/float_argument.hh>  // for detail::parseFloatSingle
#include <expected>
#include <system_error>

namespace argon {

// ---- PositionalArgument<float> ----

template <>
struct PositionalArgument<float> : ArgumentTag {
  static constexpr auto type = ArgumentType::positional;
  using value_type = float;

  [[nodiscard]] constexpr auto value() const noexcept -> const float& { return value_; }
  [[nodiscard]] constexpr auto seen() const noexcept -> bool { return seen_; }

  template <class>
  friend class Parser;

 protected:
  auto parse(std::string_view sv) -> std::expected<void, std::error_code> {
    return detail::parseFloatSingle(sv, value_);
  }

  constexpr auto markSeen() noexcept -> void { seen_ = true; }

 private:
  float value_{};
  bool seen_ = false;
};

// ---- PositionalArgument<double> ----

template <>
struct PositionalArgument<double> : ArgumentTag {
  static constexpr auto type = ArgumentType::positional;
  using value_type = double;

  [[nodiscard]] constexpr auto value() const noexcept -> const double& { return value_; }
  [[nodiscard]] constexpr auto seen() const noexcept -> bool { return seen_; }

  template <class>
  friend class Parser;

 protected:
  auto parse(std::string_view sv) -> std::expected<void, std::error_code> {
    return detail::parseFloatSingle(sv, value_);
  }

  constexpr auto markSeen() noexcept -> void { seen_ = true; }

 private:
  double value_{};
  bool seen_ = false;
};

// ---- Aliases ----

using FloatPositional = PositionalArgument<float>;
using DoublePositional = PositionalArgument<double>;

}  // namespace argon
