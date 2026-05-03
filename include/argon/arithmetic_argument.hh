#pragma once

#include <argon/argument.hh>
#include <cstdint>
#include <expected>
#include <span>
#include <system_error>
#include <vector>

namespace argon::detail {

// Register all ArithmeticParseable types so that parsable_type<T> is satisfied,
// enabling the arithmetic path in the primary Arg template.
template <ArithmeticParseable T>
inline constexpr bool is_parsable<T> = true;

}  // namespace argon::detail

namespace argon {

// ---- Arg<std::vector<T>, ...> for any arithmetic type (one-or-more values) ----

template <detail::ArithmeticParseable T, StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<std::vector<T>, LongOpt, ShortOpt> : public ArgBase<std::vector<T>, LongOpt, ShortOpt> {
  using Base = ArgBase<std::vector<T>, LongOpt, ShortOpt>;
  template <class>
  friend class Parser;

  // Explicit requirement so default construction is unambiguous.
  constexpr explicit Arg(Requirement requirement, detail::Nargs nargs = nargs::one_or_more)
      : Base(requirement), nargs_(nargs) {}

  constexpr explicit Arg(detail::Nargs nargs = nargs::one_or_more)
      : Base(optional), nargs_(nargs) {}

 protected:
  auto parse(std::span<const std::string_view> sv) -> std::expected<void, std::error_code> {
    auto& out = this->valueRef();
    out.clear();
    for (const auto& s : sv) {
      T val{};
      if (auto r = detail::parseArithmetic(s, val); !r) return r;
      out.push_back(val);
    }
    return {};
  }

  auto validate() -> std::expected<void, std::error_code> { return {}; }
  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }

 private:
  detail::Nargs nargs_ = nargs::one_or_more;
};

// ---- Single-value aliases ----

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using IntArg = Arg<int, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using Int32Arg = Arg<int32_t, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using Int64Arg = Arg<int64_t, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using Uint32Arg = Arg<uint32_t, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using Uint64Arg = Arg<uint64_t, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using FloatArg = Arg<float, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using DoubleArg = Arg<double, LongOpt, ShortOpt>;

// ---- List aliases ----

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using IntListArg = Arg<std::vector<int>, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using Int32ListArg = Arg<std::vector<int32_t>, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using Int64ListArg = Arg<std::vector<int64_t>, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using Uint32ListArg = Arg<std::vector<uint32_t>, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using Uint64ListArg = Arg<std::vector<uint64_t>, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using FloatListArg = Arg<std::vector<float>, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using DoubleListArg = Arg<std::vector<double>, LongOpt, ShortOpt>;

}  // namespace argon
