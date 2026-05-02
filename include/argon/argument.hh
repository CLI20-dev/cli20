#pragma once

#include <argon/string_literal.hh>

namespace argon {

struct ArgumentTag {};

enum class ArgumentType : std::uint8_t {
  flag,
  option,
  positional,
  command,
};

namespace detail {

template <StringLiteral Name>
[[nodiscard]]
constexpr auto IsValidLongOpt() noexcept -> bool {
  constexpr auto size = Name.size();

  if constexpr (size == 0) {
    return false;
  }
  auto is_alpha = [&](char c) consteval -> bool { return ('a' <= c && c <= 'z'); };
  auto is_digit = [](char c) consteval -> bool { return '0' <= c && c <= '9'; };
  auto is_alnum = [&](char c) consteval -> bool { return is_alpha(c) || is_digit(c); };

  if (!is_alpha(Name[0])) {
    return false;
  }
  bool previous_is_hyphen = false;
  for (std::size_t i = 1; i < size; ++i) {
    const char c = Name[i];
    if (c == '-') {
      if (previous_is_hyphen) {
        return false;
      }
      previous_is_hyphen = true;
      continue;
    }
    if (!is_alnum(c)) {
      return false;
    }
    previous_is_hyphen = false;
  }

  return !previous_is_hyphen;
}

constexpr auto IsShortOptionName(char Name) noexcept -> bool {
  return ('a' <= Name && Name <= 'z') || ('A' <= Name && Name <= 'Z');
}

constexpr auto IsValidShortOpt(char Name) noexcept -> bool {
  return IsShortOptionName(Name) || Name == '\0';
}

template <StringLiteral Name>
[[nodiscard]]
consteval auto IsCommandName() noexcept {
  return IsValidLongOpt<Name>();
}

struct Nargs {
  std::size_t min;
  std::optional<std::size_t> max;
};

}  // namespace detail

enum class Requirement : std::uint8_t {
  optional,
  required,
};

namespace nargs {

inline constexpr detail::Nargs none{.min = 0, .max = 0};
inline constexpr detail::Nargs one{.min = 1, .max = 1};
inline constexpr detail::Nargs optional{.min = 0, .max = 1};
inline constexpr detail::Nargs zero_or_more{.min = 0, .max = std::nullopt};
inline constexpr detail::Nargs one_or_more{.min = 1, .max = std::nullopt};

template <std::size_t N>
inline constexpr detail::Nargs exactly{.min = N, .max = N};

template <std::size_t Min, std::size_t Max>
inline constexpr detail::Nargs between{.min = Min, .max = Max};

}  // namespace nargs

inline constexpr Requirement optional = Requirement::optional;
inline constexpr Requirement required = Requirement::required;

template <typename ValueT, StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidShortOpt(ShortOpt) && detail::IsValidLongOpt<LongOpt>())
struct ArgBase : ArgumentTag {
  static constexpr auto type = ArgumentType::option;
  using value_type = ValueT;

  static constexpr auto long_opt = LongOpt;
  static constexpr char short_opt = ShortOpt;

  constexpr explicit ArgBase(Requirement requirement = optional) : requirement_(requirement) {}

  [[nodiscard]] static constexpr auto longOpt() noexcept -> std::string_view {
    return LongOpt.view();
  }
  [[nodiscard]] static constexpr auto shortOpt() noexcept -> char { return ShortOpt; }
  [[nodiscard]] constexpr auto requirement() const noexcept -> Requirement { return requirement_; }
  [[nodiscard]] constexpr auto isRequired() const noexcept -> bool {
    return requirement_ == required;
  }
  [[nodiscard]] constexpr auto value() const noexcept -> const ValueT& { return value_; }
  [[nodiscard]] constexpr auto seen() const noexcept -> bool { return occurrence_count_ != 0; }
  [[nodiscard]] constexpr auto occurrenceCount() const noexcept -> std::size_t {
    return occurrence_count_;
  }

 protected:
  constexpr auto valueRef() noexcept -> ValueT& { return value_; }
  constexpr auto markSeen() noexcept -> void { ++occurrence_count_; }

 private:
  Requirement requirement_ = optional;
  std::size_t occurrence_count_ = 0;
  ValueT value_ = {};
};

template <typename ValueT>
struct PositionalArgument : ArgumentTag {
  static constexpr auto type = ArgumentType::positional;
  using value_type = ValueT;

  [[nodiscard]] constexpr auto value() const noexcept -> const ValueT& { return value_; }

 protected:
  [[nodiscard]] constexpr auto valueRef() noexcept -> ValueT& { return value_; }

 private:
  ValueT value_ = {};
};

template <StringLiteral LongOpt, char ShortOpt>
struct Flag : ArgumentTag {
  static constexpr auto type = ArgumentType::flag;

 protected:
  [[nodiscard]] static constexpr auto longOpt() -> std::string_view { return LongOpt.view(); }
  [[nodiscard]] static constexpr auto shortOpt() -> char { return ShortOpt; }

  template <class>
  friend class Parser;
};

template <class>
class Parser;

template <typename ValueT, StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg : public ArgBase<ValueT, LongOpt, ShortOpt> {
  using ArgBase<ValueT, LongOpt, ShortOpt>::ArgBase;
};

template <class T, StringLiteral CommandName>
  requires(detail::IsCommandName<CommandName>())
struct Command : ArgumentTag {
  static constexpr auto type = ArgumentType::command;
  T args;

 protected:
  [[nodiscard]] static constexpr auto commandName() -> std::string_view {
    return CommandName.view();
  }
  template <class>
  friend class Parser;
};

}  // namespace argon
