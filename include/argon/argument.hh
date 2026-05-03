#pragma once

#include <argon/string_literal.hh>
#include <cstdint>
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "argon/error.hh"

namespace argon {

template <class>
class Parser;

// Whitelist of all value types that Arg<T> supports.
// The primary Arg template is constrained to !parsable_type<T> so it only
// fires a static_assert for truly unsupported types.
template <typename T>
concept parsable_type =
    std::same_as<T, int> || std::same_as<T, int32_t> || std::same_as<T, int64_t> ||
    std::same_as<T, uint32_t> || std::same_as<T, uint64_t> || std::same_as<T, float> ||
    std::same_as<T, double> || std::same_as<T, bool> || std::same_as<T, std::string> ||
    std::same_as<T, std::vector<int>> || std::same_as<T, std::vector<int32_t>> ||
    std::same_as<T, std::vector<int64_t>> || std::same_as<T, std::vector<uint32_t>> ||
    std::same_as<T, std::vector<uint64_t>> || std::same_as<T, std::vector<float>> ||
    std::same_as<T, std::vector<double>> || std::same_as<T, std::vector<bool>> ||
    std::same_as<T, std::vector<std::string>>;

struct ArgumentTag {};

enum class ArgumentType : std::uint8_t {
  flag,
  option,
  positional,
  command,
  description,
};

enum class Requirement : std::uint8_t {
  optional,
  required,
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

constexpr auto IsValidShortOpt(char Name) noexcept -> bool {
  return ('a' <= Name && Name <= 'z') || ('A' <= Name && Name <= 'Z') || Name == '\0';
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

// ---- Param<T>: configuration bundle for Arg constructors ----
//
// Pass a Param<T> to any Arg constructor to configure requirement, nargs,
// validator, and description in one place using designated initializers:
//
//   IntArg<"port", 'p'> port{{
//       .requirement = argon::required,
//       .validator   = argon::validate::range<1, 65535>,
//       .description = "Port number"
//   }};
//
// The <T> is inferred from context — no need to write Param<int> explicitly.
//
// validator must be a callable (const T&) -> std::expected<void, std::string>.
// On failure the string becomes the detail field of a ParseError with
// ErrorCode::validation_failed.

template <typename T>
struct Param {
  Requirement requirement = optional;
  detail::Nargs nargs = nargs::one;
  std::function<std::expected<void, std::string>(const T&)> validator;
  std::string_view description;
};

// Partial specialisation for list arguments: nargs defaults to one_or_more.
template <typename T>
struct Param<std::vector<T>> {
  Requirement requirement = optional;
  detail::Nargs nargs = nargs::one_or_more;
  std::function<std::expected<void, std::string>(const std::vector<T>&)> validator;
  std::string_view description;
};

// Detailed description for a parser or sub-command.
//
// Add exactly one member of this type to an Arguments struct to attach a
// longer explanation that appears in formatHelp() immediately after the
// usage line.  This is the right place for multi-sentence descriptions.
//
//   struct BuildArgs {
//     argon::description desc{
//         "Compile all source files in the current directory. "
//         "Defaults to a debug build unless --target is set."};
//     argon::StrArg<"target", 't'> target{"Build target"};
//   };
//
// Relationship with Command<SubArgs, "name">:
//   - The description on the Command<> field is a *simple one-line summary*
//     shown in the parent's "Commands:" listing.
//   - The argon::description member inside SubArgs is the *detailed*
//     description shown when the sub-command's own help is rendered.
//   - If the Command<> field has no description, the argon::description
//     member is used as a fallback in the parent listing as well.
struct Description : ArgumentTag {
  static constexpr auto type = ArgumentType::description;

  constexpr Description(std::string_view text = {}) : text_(text) {}

  [[nodiscard]] constexpr auto text() const noexcept -> std::string_view { return text_; }

 private:
  std::string_view text_;
};

using description = Description;

template <typename ValueT, StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidShortOpt(ShortOpt) && detail::IsValidLongOpt<LongOpt>())
struct ArgBase : ArgumentTag {
  static constexpr auto type = ArgumentType::option;
  using value_type = ValueT;

  static constexpr auto long_opt = LongOpt;
  static constexpr char short_opt = ShortOpt;

  constexpr explicit ArgBase(Requirement req = optional, std::string_view desc = {})
      : requirement_(req), description_(desc) {}
  constexpr explicit ArgBase(std::string_view desc) : description_(desc) {}
  explicit ArgBase(Param<ValueT> p)
      : requirement_(p.requirement), description_(p.description),
        nargs_override_(p.nargs) {
    if (p.validator) validator_ = std::move(p.validator);
  }

  [[nodiscard]] static constexpr auto longOpt() noexcept -> std::string_view {
    return LongOpt.view();
  }
  [[nodiscard]] static constexpr auto shortOpt() noexcept -> char { return ShortOpt; }
  [[nodiscard]] constexpr auto requirement() const noexcept -> Requirement { return requirement_; }
  [[nodiscard]] constexpr auto isRequired() const noexcept -> bool {
    return requirement_ == required;
  }
  [[nodiscard]] constexpr auto value() const noexcept -> const ValueT& { return value_; }
  [[nodiscard]] constexpr auto provided() const noexcept -> bool { return occurrence_count_ != 0; }
  [[nodiscard]] constexpr auto occurrenceCount() const noexcept -> std::size_t {
    return occurrence_count_;
  }
  [[nodiscard]] constexpr auto description() const noexcept -> std::string_view {
    return description_;
  }

 protected:
  constexpr auto valueRef() noexcept -> ValueT& { return value_; }
  constexpr auto markProvided() noexcept -> void { ++occurrence_count_; }

  std::optional<detail::Nargs> nargs_override_;

  auto validate() -> std::expected<void, ParseError> {
    if (!validator_ || !*validator_) return {};
    auto r = (*validator_)(value_);
    if (!r)
      return std::unexpected(ParseError{.code = ErrorCode::validation_failed,
                                        .kind = ErrorKind::validation,
                                        .subject = std::string(LongOpt.view()),
                                        .detail = std::move(r).error()});
    return {};
  }

 private:
  Requirement requirement_ = optional;
  std::string_view description_;
  std::size_t occurrence_count_ = 0;
  ValueT value_ = {};
  // std::optional used here so the default constructor is constexpr (empty
  // optional does not construct std::function, which has no constexpr ctor).
  std::optional<std::function<std::expected<void, std::string>(const ValueT&)>> validator_;
};

template <typename ValueT>
struct PositionalArgument : ArgumentTag {
  static constexpr auto type = ArgumentType::positional;
  using value_type = ValueT;

  constexpr PositionalArgument(Requirement req = optional, std::string_view desc = {})
      : requirement_(req), description_(desc) {}
  constexpr PositionalArgument(std::string_view desc) : description_(desc) {}

  [[nodiscard]] constexpr auto value() const noexcept -> const ValueT& { return value_; }
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
  [[nodiscard]] constexpr auto valueRef() noexcept -> ValueT& { return value_; }
  constexpr auto markProvided() noexcept -> void { provided_ = true; }

 private:
  Requirement requirement_ = optional;
  std::string_view description_;
  ValueT value_{};
  bool provided_ = false;
};

template <typename ValueT, StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Arg : public ArgBase<ValueT, LongOpt, ShortOpt> {
  static_assert([]<typename T = ValueT>() consteval -> auto { return parsable_type<T>; }(),
                "Arg<ValueT>: unsupported value type. "
                "Supported: int/int32_t/int64_t/uint32_t/uint64_t/float/double "
                "(arithmetic_argument.hh); std::string (string_argument.hh); "
                "bool (bool_argument.hh); std::vector<T> of any of the above.");
};

template <class T, StringLiteral CommandName>
  requires(detail::IsCommandName<CommandName>())
struct Command : ArgumentTag, public T {
  using args_type = T;
  static constexpr auto type = ArgumentType::command;

  // Simple one-line summary shown in the parent's "Commands:" listing.
  // For a detailed multi-sentence description rendered in the sub-command's
  // own help output, add an argon::description member to T instead.
  constexpr Command(std::string_view desc = {}) : description_(desc) {}

  [[nodiscard]] constexpr auto provided() const noexcept -> bool { return provided_; }
  [[nodiscard]] constexpr auto description() const noexcept -> std::string_view {
    return description_;
  }

 protected:
  [[nodiscard]] static constexpr auto commandName() -> std::string_view {
    return CommandName.view();
  }
  constexpr auto markProvided() noexcept -> void { provided_ = true; }

  template <class>
  friend class Parser;

 private:
  std::string_view description_;
  bool provided_ = false;
};

}  // namespace argon
