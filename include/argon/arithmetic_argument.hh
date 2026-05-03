#pragma once

#include <argon/argument.hh>
#include <charconv>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <system_error>
#include <vector>

namespace argon {

namespace detail {

// Parse a single arithmetic value from a string_view via std::from_chars.
// Returns result_out_of_range on overflow, invalid_argument on bad input / partial parse.
template <typename T>
  requires((std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>) || std::floating_point<T>)
auto parseArithmetic(std::string_view sv, T& out) -> std::expected<void, std::error_code> {
  const char* first = sv.data();
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  const char* last = first + sv.size();
  std::from_chars_result res{};
  if constexpr (std::floating_point<T>) {
    res = std::from_chars(first, last, out, std::chars_format::general);
  } else {
    res = std::from_chars(first, last, out);
  }
  if (res.ec != std::errc() || res.ptr != last) {
    return std::unexpected(res.ec != std::errc()
                               ? std::make_error_code(res.ec)
                               : std::make_error_code(std::errc::invalid_argument));
  }
  return {};
}

// Shared implementation base for single-value arithmetic Arg specializations.
template <typename T, StringLiteral LongOpt, char ShortOpt>
struct ArithmeticArgImpl : public ArgBase<T, LongOpt, ShortOpt> {
  using ArgBase<T, LongOpt, ShortOpt>::ArgBase;
  template <class>
  friend class Parser;

 protected:
  auto parse(std::span<const std::string_view> sv) -> std::expected<void, std::error_code> {
    return parseArithmetic(sv[0], this->valueRef());
  }

  [[nodiscard]] auto nargs() const noexcept -> Nargs { return nargs_; }

 private:
  Nargs nargs_ = nargs::one;
};

// Shared implementation base for single-value arithmetic PositionalArgument specializations.
template <typename T>
struct ArithmeticPositionalImpl : ArgumentTag {
  static constexpr auto type = ArgumentType::positional;
  using value_type = T;

  constexpr ArithmeticPositionalImpl(Requirement requirement = optional)
      : requirement_(requirement) {}

  [[nodiscard]] constexpr auto value() const noexcept -> const T& { return value_; }
  [[nodiscard]] constexpr auto provided() const noexcept -> bool { return provided_; }
  [[nodiscard]] constexpr auto isRequired() const noexcept -> bool {
    return requirement_ == Requirement::required;
  }

  template <class>
  friend class Parser;

 protected:
  [[nodiscard]] constexpr auto valueRef() noexcept -> T& { return value_; }
  constexpr auto markProvided() noexcept -> void { provided_ = true; }

  auto parse(std::string_view sv) -> std::expected<void, std::error_code> {
    return parseArithmetic(sv, value_);
  }

 private:
  Requirement requirement_ = optional;
  T value_{};
  bool provided_ = false;
};

}  // namespace detail

// ---- Arg<T, ...> explicit specializations for each arithmetic type ----
//
// Specializations are defined for the underlying fundamental C++ types rather than
// fixed-width typedef aliases (e.g. int rather than int32_t), since typedefs may alias
// the same underlying type on a given platform (int32_t == int on most 64-bit targets)
// and two specializations for the same type would be a redefinition error.
// The public aliases (Int32Arg, Int64Arg, etc.) resolve correctly because int32_t,
// int64_t, etc. are guaranteed to alias one of the fundamental types below.

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ARGON_ARITHMETIC_ARG_SPEC(T)                                                          \
  template <StringLiteral LongOpt, char ShortOpt>                                             \
    requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))          \
  struct Arg<T, LongOpt, ShortOpt> : public detail::ArithmeticArgImpl<T, LongOpt, ShortOpt> { \
    using detail::ArithmeticArgImpl<T, LongOpt, ShortOpt>::ArithmeticArgImpl;                 \
    template <class> /* NOLINT(bugprone-forward-declaration-namespace) */                     \
    friend class Parser;                                                                      \
  };

ARGON_ARITHMETIC_ARG_SPEC(int)
ARGON_ARITHMETIC_ARG_SPEC(long)       // NOLINT(google-runtime-int)
ARGON_ARITHMETIC_ARG_SPEC(long long)  // NOLINT(google-runtime-int)
ARGON_ARITHMETIC_ARG_SPEC(unsigned int)
ARGON_ARITHMETIC_ARG_SPEC(unsigned long)       // NOLINT(google-runtime-int)
ARGON_ARITHMETIC_ARG_SPEC(unsigned long long)  // NOLINT(google-runtime-int)
ARGON_ARITHMETIC_ARG_SPEC(float)
ARGON_ARITHMETIC_ARG_SPEC(double)

#undef ARGON_ARITHMETIC_ARG_SPEC

// ---- PositionalArgument<T> explicit specializations for each arithmetic type ----

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ARGON_ARITHMETIC_POSITIONAL_SPEC(T)                                   \
  template <>                                                                 \
  struct PositionalArgument<T> : public detail::ArithmeticPositionalImpl<T> { \
    using detail::ArithmeticPositionalImpl<T>::ArithmeticPositionalImpl;      \
    template <class>                                                          \
    friend class Parser;                                                      \
  };

ARGON_ARITHMETIC_POSITIONAL_SPEC(int)
ARGON_ARITHMETIC_POSITIONAL_SPEC(long)       // NOLINT(google-runtime-int)
ARGON_ARITHMETIC_POSITIONAL_SPEC(long long)  // NOLINT(google-runtime-int)
ARGON_ARITHMETIC_POSITIONAL_SPEC(unsigned int)
ARGON_ARITHMETIC_POSITIONAL_SPEC(unsigned long)       // NOLINT(google-runtime-int)
ARGON_ARITHMETIC_POSITIONAL_SPEC(unsigned long long)  // NOLINT(google-runtime-int)
ARGON_ARITHMETIC_POSITIONAL_SPEC(float)
ARGON_ARITHMETIC_POSITIONAL_SPEC(double)

#undef ARGON_ARITHMETIC_POSITIONAL_SPEC

// ---- Arg<std::vector<T>, ...> for arithmetic element types (one-or-more values) ----

template <typename T, StringLiteral LongOpt, char ShortOpt>
  requires((std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>) ||
           std::floating_point<T>) &&
          (detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<std::vector<T>, LongOpt, ShortOpt> : public ArgBase<std::vector<T>, LongOpt, ShortOpt> {
  using Base = ArgBase<std::vector<T>, LongOpt, ShortOpt>;
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

// ---- Positional aliases ----

using IntPositional = PositionalArgument<int>;
using Int32Positional = PositionalArgument<int32_t>;
using Int64Positional = PositionalArgument<int64_t>;
using Uint32Positional = PositionalArgument<uint32_t>;
using Uint64Positional = PositionalArgument<uint64_t>;
using FloatPositional = PositionalArgument<float>;
using DoublePositional = PositionalArgument<double>;

using IntPositionalArg = IntPositional;
using Int32PositionalArg = Int32Positional;
using Int64PositionalArg = Int64Positional;
using Uint32PositionalArg = Uint32Positional;
using Uint64PositionalArg = Uint64Positional;
using FloatPositionalArg = FloatPositional;
using DoublePositionalArg = DoublePositional;

using IntPosArg = IntPositional;
using Int32PosArg = Int32Positional;
using Int64PosArg = Int64Positional;
using Uint32PosArg = Uint32Positional;
using Uint64PosArg = Uint64Positional;
using FloatPosArg = FloatPositional;
using DoublePosArg = DoublePositional;

}  // namespace argon
