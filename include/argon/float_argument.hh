#pragma once

#include <argon/argument.hh>
#include <charconv>
#include <expected>
#include <span>
#include <system_error>
#include <vector>

namespace argon {

namespace detail {

// Shared helper: parse a single floating-point value from a string_view.
// Returns the actual from_chars error code on failure (e.g. result_out_of_range for overflow),
// or invalid_argument when from_chars succeeds but did not consume the whole string.
template <std::floating_point T>
auto parseFloatSingle(std::string_view sv, T& out) -> std::expected<void, std::error_code> {
  auto res = std::from_chars(sv.begin(), sv.end(), out, std::chars_format::general);
  if (res.ec != std::errc() || res.ptr != sv.end()) {
    return std::unexpected(res.ec != std::errc() ? std::make_error_code(res.ec)
                                                 : std::make_error_code(std::errc::invalid_argument));
  }
  return {};
}

}  // namespace detail

// ---- Arg<float, ...> ----

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<float, LongOpt, ShortOpt> : public ArgBase<float, LongOpt, ShortOpt> {
  using ArgBase<float, LongOpt, ShortOpt>::ArgBase;
  template <class>
  friend class Parser;

 protected:
  auto parse(std::span<const std::string_view> sv) -> std::expected<void, std::error_code> {
    return detail::parseFloatSingle(sv[0], this->valueRef());
  }

  auto validate() -> std::expected<void, std::error_code> { return {}; }
  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }

 private:
  detail::Nargs nargs_ = nargs::one;
};

// ---- Arg<std::vector<float>, ...> ----

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<std::vector<float>, LongOpt, ShortOpt>
    : public ArgBase<std::vector<float>, LongOpt, ShortOpt> {
  using Base = ArgBase<std::vector<float>, LongOpt, ShortOpt>;
  template <class>
  friend class Parser;

  constexpr explicit Arg(Requirement requirement,
                         detail::Nargs nargs = nargs::one_or_more)
      : Base(requirement), nargs_(nargs) {}

  constexpr explicit Arg(detail::Nargs nargs = nargs::one_or_more)
      : Base(optional), nargs_(nargs) {}

 protected:
  auto parse(std::span<const std::string_view> sv) -> std::expected<void, std::error_code> {
    auto& out = this->valueRef();
    out.clear();
    for (const auto& s : sv) {
      float val{};
      if (auto r = detail::parseFloatSingle(s, val); !r) return r;
      out.push_back(val);
    }
    return {};
  }

  auto validate() -> std::expected<void, std::error_code> { return {}; }
  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }

 private:
  detail::Nargs nargs_ = nargs::one_or_more;
};

// ---- Arg<double, ...> ----

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<double, LongOpt, ShortOpt> : public ArgBase<double, LongOpt, ShortOpt> {
  using ArgBase<double, LongOpt, ShortOpt>::ArgBase;
  template <class>
  friend class Parser;

 protected:
  auto parse(std::span<const std::string_view> sv) -> std::expected<void, std::error_code> {
    return detail::parseFloatSingle(sv[0], this->valueRef());
  }

  auto validate() -> std::expected<void, std::error_code> { return {}; }
  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }

 private:
  detail::Nargs nargs_ = nargs::one;
};

// ---- Arg<std::vector<double>, ...> ----

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<std::vector<double>, LongOpt, ShortOpt>
    : public ArgBase<std::vector<double>, LongOpt, ShortOpt> {
  using Base = ArgBase<std::vector<double>, LongOpt, ShortOpt>;
  template <class>
  friend class Parser;

  constexpr explicit Arg(Requirement requirement,
                         detail::Nargs nargs = nargs::one_or_more)
      : Base(requirement), nargs_(nargs) {}

  constexpr explicit Arg(detail::Nargs nargs = nargs::one_or_more)
      : Base(optional), nargs_(nargs) {}

 protected:
  auto parse(std::span<const std::string_view> sv) -> std::expected<void, std::error_code> {
    auto& out = this->valueRef();
    out.clear();
    for (const auto& s : sv) {
      double val{};
      if (auto r = detail::parseFloatSingle(s, val); !r) return r;
      out.push_back(val);
    }
    return {};
  }

  auto validate() -> std::expected<void, std::error_code> { return {}; }
  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }

 private:
  detail::Nargs nargs_ = nargs::one_or_more;
};

// ---- Aliases ----

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using FloatArg = Arg<float, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using FloatListArg = Arg<std::vector<float>, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using DoubleArg = Arg<double, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using DoubleListArg = Arg<std::vector<double>, LongOpt, ShortOpt>;

}  // namespace argon
