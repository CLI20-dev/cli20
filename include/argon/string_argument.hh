#pragma once

#include <argon/argument.hh>
#include <expected>
#include <functional>
#include <span>
#include <string>
#include <vector>

#include "argon/error.hh"

namespace argon {

// ---- Arg<std::string, ...> (single value) ----

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<std::string, LongOpt, ShortOpt> : public ArgBase<std::string, LongOpt, ShortOpt> {
  using ArgBase<std::string, LongOpt, ShortOpt>::ArgBase;
  template <class>
  friend class Parser;

 protected:
  auto parse(std::span<const std::string_view> sv) -> std::expected<void, ParseError> {
    this->valueRef() = std::string(sv[0]);
    return {};
  }

  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs {
    if (this->nargs_override_) return *this->nargs_override_;
    return nargs::one;
  }
};

// ---- Arg<std::vector<std::string>, ...> (one-or-more values) ----

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<std::vector<std::string>, LongOpt, ShortOpt>
    : public ArgBase<std::vector<std::string>, LongOpt, ShortOpt> {
  using Base = ArgBase<std::vector<std::string>, LongOpt, ShortOpt>;
  template <class>
  friend class Parser;

  // Non-explicit default: allows copy-init from {} in aggregate member initialization.
  constexpr Arg() = default;

  constexpr explicit Arg(Requirement req, detail::Nargs nargs = nargs::one_or_more,
                         std::string_view desc = {})
      : Base(req, desc), nargs_(nargs) {}

  // nargs has no default to avoid ambiguity with Arg() above.
  constexpr explicit Arg(detail::Nargs nargs, std::string_view desc = {})
      : Base(optional, desc), nargs_(nargs) {}

  constexpr explicit Arg(std::string_view desc) : Base(optional, desc) {}

  constexpr explicit Arg(Param<std::vector<std::string>> p) : Base(std::move(p)) {}

 protected:
  auto parse(std::span<const std::string_view> sv) -> std::expected<void, ParseError> {
    auto& out = this->valueRef();
    out.clear();
    for (const auto& s : sv) {
      out.emplace_back(s);
    }
    return {};
  }

  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs {
    if (this->nargs_override_) return *this->nargs_override_;
    return nargs_;
  }

 private:
  detail::Nargs nargs_ = nargs::one_or_more;
};

// ---- PositionalArgument<std::string> ----

template <>
struct PositionalArgument<std::string> : ArgumentTag {
  static constexpr auto type = ArgumentType::positional;
  using value_type = std::string;

  constexpr PositionalArgument(Requirement req = optional, std::string_view desc = {})
      : requirement_(req), description_(desc) {}
  constexpr PositionalArgument(std::string_view desc) : description_(desc) {}
  explicit PositionalArgument(Param<std::string> p)
      : requirement_(p.requirement), description_(p.description) {
    if (p.validator) validator_ = std::move(p.validator);
  }

  [[nodiscard]] constexpr auto value() const noexcept -> const std::string& { return value_; }
  [[nodiscard]] constexpr auto provided() const noexcept -> bool { return provided_; }
  [[nodiscard]] constexpr auto isRequired() const noexcept -> bool {
    return requirement_ == Requirement::required;
  }
  [[nodiscard]] constexpr auto description() const noexcept -> std::string_view {
    return description_;
  }

  template <class>
  friend class Parser;

 protected:
  auto parse(std::string_view sv) -> std::expected<void, ParseError> {
    value_ = std::string(sv);
    return {};
  }

  auto validate() -> std::expected<void, ParseError> {
    if (!validator_ || !*validator_) return {};
    auto r = (*validator_)(value_);
    if (!r)
      return std::unexpected(ParseError{.code = ErrorCode::validation_failed,
                                        .kind = ErrorKind::validation,
                                        .detail = std::move(r).error()});
    return {};
  }

  constexpr auto markProvided() noexcept -> void { provided_ = true; }

 private:
  Requirement requirement_ = optional;
  std::string_view description_;
  std::string value_;
  bool provided_ = false;
  std::optional<std::function<std::expected<void, std::string>(const std::string&)>> validator_;
};

// ---- Aliases ----

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using StringArg = Arg<std::string, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using StringListArg = Arg<std::vector<std::string>, LongOpt, ShortOpt>;

using StringPositional = PositionalArgument<std::string>;

// Short-form aliases
template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using StrArg = StringArg<LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using StrListArg = StringListArg<LongOpt, ShortOpt>;

using StrPositional = StringPositional;
using StrPositionalArg = StringPositional;
using StrPosArg = StringPositional;

}  // namespace argon
