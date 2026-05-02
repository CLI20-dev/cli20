#pragma once

#include <argon/argument.hh>
#include <charconv>
#include <expected>
#include <span>
#include <system_error>
#include <vector>

namespace argon {

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<int, LongOpt, ShortOpt> : public ArgBase<int, LongOpt, ShortOpt> {
  using ArgBase<int, LongOpt, ShortOpt>::ArgBase;
  template <class>
  friend class Parser;

 protected:
  auto parse(std::span<const std::string_view> sv) -> std::expected<void, std::error_code> {
    auto& out = this->valueRef();
    auto res = std::from_chars(sv[0].begin(), sv[0].end(), out);
    if (res.ec != std::errc() || res.ptr != sv[0].end()) {
      return std::unexpected(res.ec != std::errc() ? std::make_error_code(res.ec)
                                                   : std::make_error_code(std::errc::invalid_argument));
    }
    return {};
  }

  auto validate() -> std::expected<void, std::error_code> { return {}; }

  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }

 private:
  detail::Nargs nargs_ = nargs::one;
};

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<std::vector<int>, LongOpt, ShortOpt>
    : public ArgBase<std::vector<int>, LongOpt, ShortOpt> {
  using Base = ArgBase<std::vector<int>, LongOpt, ShortOpt>;
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
      int val{};
      auto res = std::from_chars(s.begin(), s.end(), val);
      if (res.ec != std::errc() || res.ptr != s.end()) {
        return std::unexpected(res.ec != std::errc() ? std::make_error_code(res.ec)
                                                   : std::make_error_code(std::errc::invalid_argument));
      }
      out.push_back(val);
    }
    return {};
  }

  auto validate() -> std::expected<void, std::error_code> { return {}; }

  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }

 private:
  detail::Nargs nargs_ = nargs::one_or_more;
};

template <StringLiteral LongOpt, char ShortOpt, std::size_t N>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<std::array<int, N>, LongOpt, ShortOpt>
    : public ArgBase<std::array<int, N>, LongOpt, ShortOpt> {
  using Base = ArgBase<std::array<int, N>, LongOpt, ShortOpt>;
  template <class>
  friend class Parser;

  constexpr explicit Arg(Requirement requirement = optional) : Base(requirement) {}

 protected:
  auto parse(std::span<const std::string_view> sv) -> std::expected<void, std::error_code> {
    auto& out = this->valueRef();
    for (std::size_t i = 0; i < N; ++i) {
      auto res = std::from_chars(sv[i].begin(), sv[i].end(), out[i]);
      if (res.ec != std::errc() || res.ptr != sv[i].end()) {
        return std::unexpected(res.ec != std::errc() ? std::make_error_code(res.ec)
                                                   : std::make_error_code(std::errc::invalid_argument));
      }
    }
    return {};
  }

  auto validate() -> std::expected<void, std::error_code> { return {}; }

  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }

 private:
  detail::Nargs nargs_ = nargs::exactly<N>;
};

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using IntArg = Arg<int, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using IntListArg = Arg<std::vector<int>, LongOpt, ShortOpt>;

}  // namespace argon
