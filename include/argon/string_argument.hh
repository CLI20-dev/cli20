#pragma once

#include <argon/argument.hh>
#include <expected>
#include <span>
#include <string>
#include <system_error>
#include <vector>

namespace argon {

// ---- Arg<std::string, ...> (single value) ----

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<std::string, LongOpt, ShortOpt> : public ArgBase<std::string, LongOpt, ShortOpt> {
  using ArgBase<std::string, LongOpt, ShortOpt>::ArgBase;
  template <class>
  friend class Parser;

 protected:
  auto parse(std::span<const std::string_view> sv) -> std::expected<void, std::error_code> {
    this->valueRef() = std::string(sv[0]);
    return {};
  }

  auto validate() -> std::expected<void, std::error_code> { return {}; }
  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }

 private:
  detail::Nargs nargs_ = nargs::one;
};

// ---- Arg<std::vector<std::string>, ...> (one-or-more values) ----

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<std::vector<std::string>, LongOpt, ShortOpt>
    : public ArgBase<std::vector<std::string>, LongOpt, ShortOpt> {
  using Base = ArgBase<std::vector<std::string>, LongOpt, ShortOpt>;
  template <class>
  friend class Parser;

  constexpr explicit Arg(Requirement requirement, detail::Nargs nargs = nargs::one_or_more)
      : Base(requirement), nargs_(nargs) {}

  constexpr explicit Arg(detail::Nargs nargs = nargs::one_or_more)
      : Base(optional), nargs_(nargs) {}

 protected:
  auto parse(std::span<const std::string_view> sv) -> std::expected<void, std::error_code> {
    auto& out = this->valueRef();
    out.clear();
    for (const auto& s : sv) {
      out.emplace_back(s);
    }
    return {};
  }

  auto validate() -> std::expected<void, std::error_code> { return {}; }
  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }

 private:
  detail::Nargs nargs_ = nargs::one_or_more;
};

// ---- PositionalArgument<std::string> ----

template <>
struct PositionalArgument<std::string> : ArgumentTag {
  static constexpr auto type = ArgumentType::positional;
  using value_type = std::string;

  constexpr PositionalArgument(Requirement requirement = optional) : requirement_(requirement) {}

  [[nodiscard]] constexpr auto value() const noexcept -> const std::string& { return value_; }
  [[nodiscard]] constexpr auto provided() const noexcept -> bool { return provided_; }
  [[nodiscard]] constexpr auto isRequired() const noexcept -> bool {
    return requirement_ == Requirement::required;
  }

  template <class>
  friend class Parser;

 protected:
  auto parse(std::string_view sv) -> std::expected<void, std::error_code> {
    value_ = std::string(sv);
    return {};
  }

  constexpr auto markProvided() noexcept -> void { provided_ = true; }

 private:
  Requirement requirement_ = optional;
  std::string value_;
  bool provided_ = false;
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
