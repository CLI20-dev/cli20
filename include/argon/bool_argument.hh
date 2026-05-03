#pragma once

#include <argon/argument.hh>
#include <expected>
#include <span>
#include <string_view>
#include <vector>

#include "argon/error.hh"

namespace argon {

namespace detail {

inline auto parseBool(std::string_view sv, bool& out) -> std::expected<void, ParseError> {
  if (sv == "true") {
    out = true;
    return {};
  }
  if (sv == "false") {
    out = false;
    return {};
  }
  return std::unexpected(ParseError{.code = ErrorCode::conversion_error,
                                    .kind = ErrorKind::conversion,
                                    .subject = std::string(sv)});
}

}  // namespace detail

// ---- Arg<bool, ...> (single value, accepts "true" or "false") ----

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<bool, LongOpt, ShortOpt> : public ArgBase<bool, LongOpt, ShortOpt> {
  using ArgBase<bool, LongOpt, ShortOpt>::ArgBase;
  template <class>
  friend class Parser;

 protected:
  auto parse(std::span<const std::string_view> sv) -> std::expected<void, ParseError> {
    return detail::parseBool(sv[0], this->valueRef());
  }

  auto validate() -> std::expected<void, ParseError> { return {}; }
  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }

 private:
  detail::Nargs nargs_ = nargs::one;
};

// ---- PositionalArgument<bool> ----

template <>
struct PositionalArgument<bool> : ArgumentTag {
  static constexpr auto type = ArgumentType::positional;
  using value_type = bool;

  constexpr PositionalArgument(Requirement requirement = optional) : requirement_(requirement) {}

  [[nodiscard]] constexpr auto value() const noexcept -> bool { return value_; }
  [[nodiscard]] constexpr auto provided() const noexcept -> bool { return provided_; }
  [[nodiscard]] constexpr auto isRequired() const noexcept -> bool {
    return requirement_ == Requirement::required;
  }

  template <class>
  friend class Parser;

 protected:
  auto parse(std::string_view sv) -> std::expected<void, ParseError> {
    return detail::parseBool(sv, value_);
  }

  constexpr auto markProvided() noexcept -> void { provided_ = true; }

 private:
  Requirement requirement_ = optional;
  bool value_ = false;
  bool provided_ = false;
};

// ---- Arg<std::vector<bool>, ...> (one-or-more values) ----

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<std::vector<bool>, LongOpt, ShortOpt>
    : public ArgBase<std::vector<bool>, LongOpt, ShortOpt> {
  using Base = ArgBase<std::vector<bool>, LongOpt, ShortOpt>;
  template <class>
  friend class Parser;

  constexpr explicit Arg(Requirement requirement, detail::Nargs nargs = nargs::one_or_more)
      : Base(requirement), nargs_(nargs) {}

  constexpr explicit Arg(detail::Nargs nargs = nargs::one_or_more)
      : Base(optional), nargs_(nargs) {}

 protected:
  auto parse(std::span<const std::string_view> sv) -> std::expected<void, ParseError> {
    auto& out = this->valueRef();
    out.clear();
    for (const auto& s : sv) {
      bool val = false;
      if (auto r = detail::parseBool(s, val); !r) return r;
      out.push_back(val);
    }
    return {};
  }

  auto validate() -> std::expected<void, ParseError> { return {}; }
  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }

 private:
  detail::Nargs nargs_ = nargs::one_or_more;
};

// ---- Aliases ----

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using BoolArg = Arg<bool, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using BoolListArg = Arg<std::vector<bool>, LongOpt, ShortOpt>;

using BoolPositional = PositionalArgument<bool>;
using BoolPositionalArg = BoolPositional;
using BoolPosArg = BoolPositional;

}  // namespace argon
