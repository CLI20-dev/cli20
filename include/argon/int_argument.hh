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

 protected:
  auto parse(std::span<std::string_view> sv)
      -> std::expected<std::span<std::string_view>, std::error_code> {
    auto res = std::from_chars(sv[0].begin(), sv[0].end(), this->valueRef());
    if (res.ec != std::errc()) {
      return std::unexpected(std::make_error_code(res.ec));
    }
    return sv.subspan(1);
  }
  auto validate() -> std::expected<void, std::error_code> { return {}; }
};

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<std::vector<int>, LongOpt, ShortOpt>
    : public ArgBase<std::vector<int>, LongOpt, ShortOpt> {
  using Base = ArgBase<std::vector<int>, LongOpt, ShortOpt>;

  constexpr explicit Arg(Requirement requirement = optional,
                         detail::Nargs nargs = nargs::one_or_more)
      : Base(requirement), nargs_(nargs) {}

  constexpr explicit Arg(detail::Nargs nargs = nargs::one_or_more)
      : Base(optional), nargs_(nargs) {}

 protected:
  auto parse(std::span<const std::string_view> sv)
      -> std::expected<std::span<const std::string_view>, std::error_code> {
    auto& out = this->valueRef();
    out.clear();

    std::size_t i = 0;
    for (i = 0; i < sv.size(); ++i) {
      auto res = std::from_chars(sv[i].begin(), sv[i].end(), out.emplace_back());
      if (res.ec != std::errc()) {
        return std::unexpected(std::make_error_code(res.ec));
      }
    }

    return sv.subspan(i);
  }

  auto validate() -> std::expected<void, std::error_code> { return {}; }

  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }

 private:
  detail::Nargs nargs_ = nargs::one_or_more;
};

template <StringLiteral LongOpt, char ShortOpt, size_t N>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<std::array<int, N>, LongOpt, ShortOpt>
    : public ArgBase<std::array<int, N>, LongOpt, ShortOpt> {
  using Base = ArgBase<std::array<int, N>, LongOpt, ShortOpt>;

  constexpr explicit Arg(Requirement requirement = optional) : Base(requirement) {}

 protected:
  auto parse(std::span<const std::string_view> sv)
      -> std::expected<std::span<const std::string_view>, std::error_code> {
    auto& out = this->valueRef();
    out.clear();

    std::size_t i = 0;
    for (i = 0; i < sv.size(); ++i) {
      auto res = std::from_chars(sv[i].begin(), sv[i].end(), out.emplace_back());
      if (res.ec != std::errc()) {
        return std::unexpected(std::make_error_code(res.ec));
      }
    }

    return sv.subspan(i);
  }

  auto validate() -> std::expected<void, std::error_code> { return {}; }

  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }

 private:
  detail::Nargs nargs_ = nargs::exactly<N>;
};

template <size_t N, StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg<std::array<int, N>, LongOpt, ShortOpt> : public ArgBase<int, LongOpt, ShortOpt> {
  using ArgBase<int, LongOpt, ShortOpt>::ArgBase;

 protected:
  auto parse(std::span<std::string_view> sv)
      -> std::expected<std::span<std::string_view>, std::error_code> {
    auto res = std::from_chars(sv[0].begin(), sv[0].end(), this->valueRef());
    if (res.ec != std::errc()) {
      return std::unexpected(std::make_error_code(res.ec));
    }
    return sv.subspan(1);
  }
  auto validate() -> std::expected<void, std::error_code> { return {}; }
};

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using IntArg = Arg<int, LongOpt, ShortOpt>;

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using IntListArg = Arg<std::vector<int>, LongOpt, ShortOpt>;

};  // namespace argon
