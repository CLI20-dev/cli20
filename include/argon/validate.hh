#pragma once

#include <expected>
#include <initializer_list>
#include <string>
#include <vector>

// Built-in validators for use with Param<T>::validator.
//
// All validators have the signature:
//   (const T&) -> std::expected<void, std::string>
//
// Usage with designated initializers:
//
//   IntArg<"port", 'p'> port{{
//       .requirement = argon::required,
//       .validator   = argon::validate::range<1, 65535>,
//       .description = "Port number"
//   }};
//
//   StrArg<"level"> level{{
//       .validator   = argon::validate::one_of({"debug", "info", "warn"}),
//       .description = "Log level"
//   }};

namespace argon::validate {

// Passes iff value > 0 (for any type with operator> and default-constructible zero).
template <typename T>
auto positive(const T& value) -> std::expected<void, std::string> {
  if (value > T{}) return {};
  return std::unexpected(std::string("must be positive"));
}

// Passes iff value >= 0.
template <typename T>
auto non_negative(const T& value) -> std::expected<void, std::string> {
  if (value >= T{}) return {};
  return std::unexpected(std::string("must be non-negative"));
}

// Passes iff Min <= value <= Max.
// The template arguments set the bounds; T is deduced at call site.
//
//   .validator = argon::validate::range<1, 65535>
template <auto Min, auto Max>
auto range(const decltype(Min)& value) -> std::expected<void, std::string> {
  if (value >= Min && value <= Max) return {};
  return std::unexpected("must be in range [" + std::to_string(Min) + ", " +
                         std::to_string(Max) + "]");
}

// Passes iff the string is non-empty.
inline auto non_empty(const std::string& value) -> std::expected<void, std::string> {
  if (!value.empty()) return {};
  return std::unexpected(std::string("must not be empty"));
}

// Returns a validator that passes iff value equals one of the given choices.
//
//   .validator = argon::validate::one_of({"debug", "info", "warn", "error"})
template <typename T>
auto one_of(std::initializer_list<T> choices) {
  return [choices = std::vector<T>(choices)](const T& value) -> std::expected<void, std::string> {
    for (const auto& c : choices) {
      if (value == c) return {};
    }
    std::string msg = "must be one of:";
    for (const auto& c : choices) msg += " " + std::string(c);
    return std::unexpected(msg);
  };
}

}  // namespace argon::validate
