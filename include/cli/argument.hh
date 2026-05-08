#pragma once

#include <algorithm>
#include <cli/meta.hh>
#include <cli/string_literal.hh>
#include <filesystem>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

#include "cli/action.hh"

namespace cli {

/** @brief Base tag for all members that may appear in an argument specification
 * struct. */
struct SpecMemberTag {};

/** @brief Tag base class for command-line option/flag fields. */
struct OptionTag : SpecMemberTag {};

/** @brief Tag base class for positional argument fields. */
struct PositionalTag : SpecMemberTag {};

/** @brief Tag base class for subcommand fields. */
struct CommandTag : SpecMemberTag {};

/** @brief Tag base class for the `Description` field that provides the CLI's
 * help text. */
struct DescriptionTag : SpecMemberTag {};

/**
 * @brief Specifies the minimum and maximum number of values an option or
 * positional accepts.
 *
 * **Sentinel value:** The default `{-1, -1}` means *unset*. An unset `Nargs`
 * is rejected by `is_valid_nargs()` and therefore also by the `ArgImpl` /
 * `PositionalImpl` template constraints — it is only a valid intermediate
 * state before a named preset (e.g. `nargs::one`) or a custom value is
 * assigned.
 *
 * **Valid ranges for a fully-initialised `Nargs`:**
 * - `min >= 0`
 * - `max >= 1`, or `max == -1` which means *unlimited* (no upper bound)
 * - `min <= max` (when `max != -1`)
 *
 * Prefer the predefined constants in the `cli::nargs` namespace over
 * constructing `Nargs` directly.
 */
struct Nargs {
  int min = -1;  ///< Minimum number of values. -1 = unset sentinel (invalid for
                 ///< use in ArgImpl/PositionalImpl); must be >= 0 when set.
  int max = -1;  ///< Maximum number of values. -1 has two meanings: *unset*
                 ///< sentinel when `min` is also -1, or *unlimited* upper bound
                 ///< when `min >= 0`.
};

/**
 * @brief Controls whether an option or positional argument is mandatory.
 *
 * When `required`, the parser returns `ErrorCode::missing_required` if the
 * argument is absent. When `optional` (the default), absence is allowed.
 */
enum class Presence { required, optional };

/** @brief Convenience constant for `Presence::required`. */
inline constexpr auto required = Presence::required;

/** @brief Convenience constant for `Presence::optional`. */
inline constexpr auto optional = Presence::optional;

/**
 * @brief Predefined `Nargs` constants for common value-count patterns.
 */
namespace nargs {

/** @brief Accepts no values; used for boolean flags. `{0, 0}` */
inline constexpr Nargs none{.min = 0, .max = 0};

/** @brief Accepts exactly one value. `{1, 1}` */
inline constexpr Nargs one{.min = 1, .max = 1};

/** @brief Accepts zero or one value. `{0, 1}` */
inline constexpr Nargs zero_or_one{.min = 0, .max = 1};

/** @brief Accepts zero or more values. `{0, -1}` */
inline constexpr Nargs zero_or_more{.min = 0, .max = -1};

/** @brief Accepts one or more values. `{1, -1}` */
inline constexpr Nargs one_or_more{.min = 1, .max = -1};

/**
 * @brief Accepts exactly `N` values.
 * @tparam N The required number of values.
 */
template <int N>
inline constexpr Nargs exactly{.min = N, .max = N};

/**
 * @brief Accepts between `Min` and `Max` values (inclusive).
 * @tparam Min Minimum number of values.
 * @tparam Max Maximum number of values.
 */
template <int Min, int Max>
inline constexpr Nargs between{.min = Min, .max = Max};

}  // namespace nargs

namespace detail {

/**
 * @brief Checks that every member of aggregate type `T` derives from
 * `SpecMemberTag`.
 *
 * @tparam T The argument specification type to validate.
 * @return `true` if all members inherit from `SpecMemberTag`.
 */
template <class T>
consteval auto members_are_derived_from_valid_cli_class() -> bool {
  return []<class... Args>(std::type_identity<std::tuple<Args...>>) consteval
             -> auto {
    return (std::derived_from<std::remove_cvref_t<Args>, SpecMemberTag> && ...);
  }(std::type_identity<
                 std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
}

/**
 * @brief Checks that all `OptionTag` members of `T` have distinct long names.
 *
 * @tparam T The argument specification type to validate.
 * @return `true` if no two options share the same long name.
 */
template <class T>
consteval auto options_have_unique_long_name() -> bool {
  std::vector<std::string_view> names;
  [&]<class... Args>(std::type_identity<std::tuple<Args...>>) consteval
      -> auto {
    (
        [&]() consteval -> auto {
          if constexpr (std::derived_from<std::remove_cvref_t<Args>,
                                          OptionTag>) {
            names.push_back(std::remove_cvref_t<Args>::name.view());
          }
        }(),
        ...);
  }(std::type_identity<
          std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
  std::ranges::sort(names);
  return std::ranges::adjacent_find(names) == names.end();
}

/**
 * @brief Checks that all `OptionTag` members of `T` have distinct short names.
 *
 * Options with no short name (`'\0'`) are excluded from the uniqueness check.
 *
 * @tparam T The argument specification type to validate.
 * @return `true` if no two options share the same non-null short name.
 */
template <class T>
consteval auto options_have_unique_short_name() -> bool {
  std::vector<char> names;
  [&]<class... Args>(std::type_identity<std::tuple<Args...>>) consteval
      -> auto {
    (
        [&]() consteval -> auto {
          if constexpr (std::derived_from<std::remove_cvref_t<Args>,
                                          OptionTag>) {
            if constexpr (std::remove_cvref_t<Args>::short_name != '\0') {
              names.push_back(std::remove_cvref_t<Args>::short_name);
            }
          }
        }(),
        ...);
  }(std::type_identity<
          std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
  std::ranges::sort(names);
  return std::ranges::adjacent_find(names) == names.end();
}

/**
 * @brief Checks that all `CommandTag` members of `T` have distinct names.
 *
 * @tparam T The argument specification type to validate.
 * @return `true` if no two subcommands share the same name.
 */
template <class T>
consteval auto commands_have_unique_long_name() -> bool {
  std::vector<std::string_view> names;
  [&]<class... Args>(std::type_identity<std::tuple<Args...>>) consteval
      -> auto {
    (
        [&]() consteval -> auto {
          if constexpr (std::derived_from<std::remove_cvref_t<Args>,
                                          CommandTag>) {
            names.push_back(std::remove_cvref_t<Args>::name.view());
          }
        }(),
        ...);
  }(std::type_identity<
          std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
  std::ranges::sort(names);
  return std::ranges::adjacent_find(names) == names.end();
}

/**
 * @brief Checks that any variadic positional appears after all fixed
 * positionals.
 *
 * A variadic positional is one where `nargs.min != nargs.max`. The constraint
 * ensures that token dispatch is unambiguous.
 *
 * @tparam T The argument specification type to validate.
 * @return `true` if no fixed positional follows a variadic positional.
 */
template <class T>
consteval auto positionals_have_variadic_at_end() {
  bool found_variadic = false;
  bool found_positional_after_variadic = false;
  [&]<class... Args>(std::type_identity<std::tuple<Args...>>) consteval
      -> auto {
    (
        [&]() consteval -> auto {
          if constexpr (std::derived_from<std::remove_cvref_t<Args>,
                                          PositionalTag>) {
            if (found_variadic) {
              found_positional_after_variadic = true;
            }
            if (std::remove_cvref_t<Args>::nargs.max !=
                std::remove_cvref_t<Args>::nargs.min) {
              found_variadic = true;
            }
          }
        }(),
        ...);
  }(std::type_identity<
          std::remove_cvref_t<decltype(as_tuple(std::declval<T>()))>>{});
  return !found_positional_after_variadic;
}

/**
 * @brief Validates the syntax of a long option name at compile time.
 *
 * Rules:
 * - Must be non-empty.
 * - Must start with a lowercase ASCII letter.
 * - May contain lowercase letters, digits, and single hyphens.
 * - Must not contain consecutive hyphens or end with a hyphen.
 *
 * @tparam Name The option name to validate (without the `--` prefix).
 * @return `true` if `Name` is a valid long option name.
 */
template <StringLiteral Name>
[[nodiscard]]
constexpr auto is_valid_long_option_name() noexcept -> bool {
  constexpr auto size = Name.size();

  if constexpr (size == 0) {
    return false;
  }
  auto is_alpha = [](char c) constexpr -> bool {
    return ('a' <= c && c <= 'z');
  };
  auto is_digit = [](char c) constexpr -> bool { return '0' <= c && c <= '9'; };
  auto is_alnum = [&](char c) constexpr -> bool {
    return is_alpha(c) || is_digit(c);
  };

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

/**
 * @brief Validates a short option name character at compile time.
 *
 * Accepts ASCII letters (upper or lower case) or `'\0'` (meaning no short name).
 *
 * @param Name The short option character to validate.
 * @return `true` if `Name` is a valid short option name.
 */
constexpr auto is_valid_short_option_name(char Name) noexcept -> bool {
  return ('a' <= Name && Name <= 'z') || ('A' <= Name && Name <= 'Z') ||
         Name == '\0';
}

/**
 * @brief Validates the syntax of a subcommand name at compile time.
 *
 * Applies the same rules as `is_valid_long_option_name`.
 *
 * @tparam Name The command name to validate.
 * @return `true` if `Name` is a valid command name.
 */
template <StringLiteral Name>
[[nodiscard]]
consteval auto is_valid_command_name() noexcept {
  return is_valid_long_option_name<Name>();
}

/**
 * @brief Validates that a `Nargs` value is not the unset sentinel `{-1, -1}`.
 *
 * @tparam NargsValue The `Nargs` value to validate.
 * @return `true` if at least one of `min` or `max` has been explicitly set.
 */
template <Nargs NargsValue>
[[nodiscard]]
consteval auto is_valid_nargs() noexcept -> bool {
  if (NargsValue.max == -1 && NargsValue.min == -1) {
    return false;
  }
  return true;
}

}  // namespace detail

/**
 * @brief Concept that validates an aggregate type as a well-formed CLI argument
 * specification.
 *
 * A type satisfies `ArgumentSpec` when all of the following hold:
 * - All members derive from `SpecMemberTag`.
 * - All option long names are unique.
 * - All option short names are unique (ignoring `'\0'`).
 * - All command names are unique.
 * - Variadic positionals appear only at the end of the positional list.
 *
 * Violations are reported as compile-time errors via `static_assert` inside
 * the individual `consteval` checks.
 *
 * @tparam T The aggregate struct to validate.
 */
template <class T>
concept ArgumentSpec = requires {
  requires detail::members_are_derived_from_valid_cli_class<T>();
  requires detail::options_have_unique_long_name<T>();
  requires detail::options_have_unique_short_name<T>();
  requires detail::commands_have_unique_long_name<T>();
  requires detail::positionals_have_variadic_at_end<T>();
};

/**
 * @brief Parameter bag used when constructing `ArgImpl` or `PositionalImpl` with
 * named fields.
 *
 * Allows brace-initialisation with named members, e.g.:
 * @code
 *   cli::Option<int, "count"> count{cli::ArgParameter<int>{
 *       .help = "Number of repetitions",
 *       .presence = cli::required,
 *       .default_value = 1,
 *   }};
 * @endcode
 *
 * @tparam T The storage value type of the argument.
 */
template <class T>
struct ArgParameter {
  std::string_view help{};  ///< Help text shown in `--help` output.
  Presence presence{Presence::optional};  ///< Whether the argument is required.
  T default_value{};  ///< Default value when the argument is absent.
  std::function<void(const T&)>
      on_parse{};  ///< Callback invoked once after parsing completes.
};

/**
 * @brief Internal implementation type for a typed command-line option or flag.
 *
 * `ArgImpl` derives from `OptionTag` and stores the parsed value, occurrence
 * counters, and the optional `on_parse` callback. It is not usually
 * instantiated directly; instead use the public aliases `Flag`, `Option`,
 * `ListOption`, `BoundOption`, `Help`, etc.
 *
 * @tparam Name      The long option name (e.g. `"verbose"`), without the `--`
 * prefix.
 * @tparam ShortName The single-character short name (e.g. `'v'`), or `'\0'` for
 * none.
 * @tparam N         The `Nargs` descriptor controlling how many values the
 * option consumes.
 * @tparam A         The action pipeline that converts raw strings to the stored
 * type.
 */
template <StringLiteral Name, char ShortName, Nargs N, Action A>
  requires requires {
    detail::is_valid_long_option_name<Name>();
    detail::is_valid_short_option_name(ShortName);
    0 <= N.min;
    -1 <= N.max;
    (N.max == -1 || N.min <= N.max);
  }
struct ArgImpl : public OptionTag {
  using value_type =
      typename std::remove_cvref_t<decltype(A.validate().second)>::type;
  ArgImpl() = default;
  ArgImpl(ArgParameter<value_type> param)
      : help(param.help),
        presence(param.presence),
        value_(param.default_value),
        on_parse_(std::move(param.on_parse)) {}

  // Convenience constructor for StoreInto variants (value_type == T*).
  explicit ArgImpl(std::remove_pointer_t<value_type>& ref)
    requires std::is_pointer_v<value_type>
      : value_(&ref) {}
  ArgImpl(std::remove_pointer_t<value_type>& ref, std::string_view help_text)
    requires std::is_pointer_v<value_type>
      : help(help_text), value_(&ref) {}

  // Bind to an external variable after construction.
  // Allows: opt.bind(var)  or  opt = &var
  auto bind(std::remove_pointer_t<value_type>& ref) -> ArgImpl&
    requires std::is_pointer_v<value_type>
  {
    value_ = &ref;
    return *this;
  }
  auto operator=(std::remove_pointer_t<value_type>* ptr) -> ArgImpl&
    requires std::is_pointer_v<value_type>
  {
    value_ = ptr;
    return *this;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  std::string_view help{};
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  cli::Presence presence{Presence::optional};
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  value_type default_value{};

  static constexpr auto name = Name;
  static constexpr auto short_name = ShortName;
  static constexpr auto nargs = N;

  // Called once when the option token is seen (before processing its values).
  auto notify_option_seen() -> void { ++occurrence_count_; }

  // Called once per value token associated with this option.
  auto invoke_action(std::string_view text, std::size_t arg_index)
      -> ActionResult<void> {
    provided_ = true;
    ActionCtx<value_type> ctx{
        .index = arg_index,
        .occurrences = occurrence_count_,
        .invoke_count = invoke_count_,
        .arg = std::ref(value_),
    };
    ++invoke_count_;
    auto result =
        decltype(A)::invoke(ctx, ActionResult<std::string_view>::ok(text));
    if (result.has_error()) {
      return ActionResult<void>::fail(std::move(result.error));
    }
    return ActionResult<void>::ok();
  }

  // Called once for flags (nargs = {0,0}) after the option token is seen.
  auto invoke_flag(std::size_t arg_index) -> ActionResult<void> {
    provided_ = true;
    ActionCtx<value_type> ctx{
        .index = arg_index,
        .occurrences = occurrence_count_,
        .invoke_count = invoke_count_,
        .arg = std::ref(value_),
    };
    ++invoke_count_;
    auto result = decltype(A)::invoke(
        ctx, ActionResult<std::string_view>::ok(std::string_view{}));
    if (result.has_error()) {
      return ActionResult<void>::fail(std::move(result.error));
    }
    return ActionResult<void>::ok();
  }

  // Called once after all value tokens for one option occurrence are processed.
  auto fire_on_parse() -> void {
    if (on_parse_) on_parse_(value_);
  }

  [[nodiscard]] auto value() const -> const value_type& { return value_; }
  [[nodiscard]] auto value() -> value_type& { return value_; }
  [[nodiscard]] auto provided() const -> bool { return provided_; }

 private:
  template <ArgumentSpec>
  friend struct Parser;

  value_type value_{};
  std::function<void(const value_type&)> on_parse_{};
  std::size_t occurrence_count_{};  // times the option token appeared
  std::size_t invoke_count_{};      // times invoke_action/invoke_flag called
  bool provided_{};
};

/**
 * @brief Internal implementation type for a typed positional argument.
 *
 * `PositionalImpl` derives from `PositionalTag` and stores the parsed value
 * along with occurrence counters and an optional `on_parse` callback.
 * It is not usually instantiated directly; use the `Positional` alias instead.
 *
 * @tparam N The `Nargs` descriptor controlling how many values the positional
 * consumes.
 * @tparam A The action pipeline that converts raw strings to the stored type.
 */
template <Nargs N, Action A>
  requires requires {
    0 <= N.min;
    -1 <= N.max;
    (N.max == -1 || N.min <= N.max);
  }
struct PositionalImpl : public PositionalTag {
  using value_type =
      typename std::remove_cvref_t<decltype(A.validate().second)>::type;

  PositionalImpl() = default;
  PositionalImpl(ArgParameter<value_type> param)
      : help(param.help),
        presence(param.presence),
        value_(param.default_value),
        on_parse_(std::move(param.on_parse)) {}
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  std::string_view help{};
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  cli::Presence presence{Presence::optional};
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  value_type default_value{};

  static constexpr auto nargs = N;

  // Called once per value token for this positional.
  auto invoke_action(std::string_view text, std::size_t arg_index)
      -> ActionResult<void> {
    provided_ = true;
    ActionCtx<value_type> ctx{
        .index = arg_index,
        .occurrences = occurrence_count_,
        .invoke_count = invoke_count_,
        .arg = std::ref(value_),
    };
    ++occurrence_count_;
    ++invoke_count_;
    auto result =
        decltype(A)::invoke(ctx, ActionResult<std::string_view>::ok(text));
    if (result.has_error()) {
      return ActionResult<void>::fail(std::move(result.error));
    }
    return ActionResult<void>::ok();
  }

  // Called once after all tokens for this positional are consumed.
  auto fire_on_parse() -> void {
    if (on_parse_) on_parse_(value_);
  }

  [[nodiscard]] auto value() const -> const value_type& { return value_; }
  [[nodiscard]] auto value() -> value_type& { return value_; }
  [[nodiscard]] auto provided() const -> bool { return provided_; }

 private:
  template <ArgumentSpec>
  friend struct Parser;

  value_type value_{};
  std::function<void(const value_type&)> on_parse_{};
  std::size_t occurrence_count_{};  // times a value was provided
  std::size_t
      invoke_count_{};  // times invoke_action was called (same for positionals)
  bool provided_{};
};

/**
 * @brief A `DescriptionTag` field that carries the top-level CLI description.
 *
 * Add an instance of `Description` as a member of your argument specification
 * struct to provide a paragraph that appears in the help output between the
 * usage line and the options/positionals sections:
 * @code
 *   struct MyArgs {
 *     cli::Description description{"Does something useful."};
 *     cli::Flag<"verbose"> verbose;
 *   };
 * @endcode
 */
struct Description : public std::string, DescriptionTag {};

/**
 * @brief Parameter bag used when constructing a `Command` field.
 */
struct CommandParameter {
  std::string_view
      help{};  ///< Brief description shown in the parent command's help.
};

/**
 * @brief A named subcommand that nests another argument specification.
 *
 * `Command` inherits from both the argument specification type `T` (to hold
 * its parsed fields) and `CommandTag` (so the parser identifies it as a
 * subcommand). Use `provided()` to check whether the subcommand was actually
 * invoked at runtime.
 *
 * @tparam Name The subcommand name (e.g. `"build"`).
 * @tparam T    The argument specification type for the subcommand's own options.
 */
template <StringLiteral Name, ArgumentSpec T>
  requires requires { detail::is_valid_command_name<Name>(); }
struct Command : public T, public CommandTag {
  using T::T;
  using argument_type = T;

  Command() = default;
  Command(CommandParameter param) : help_(param.help) {}
  static constexpr auto name = Name;
  static constexpr auto command_name() -> std::string_view {
    return Name.view();
  }
  [[nodiscard]] auto provided() const -> bool { return provided_; }
  [[nodiscard]] auto help_text() const -> std::string_view { return help_; }
  auto mark_provided() -> void { provided_ = true; }

 private:
  std::string_view help_{};
  bool provided_{};
};

namespace detail {

template <class T>
struct ActionFor;

template <>
struct ActionFor<std::string> {
  inline static constexpr auto set_once = conversion::string | pack::set_once;
  inline static constexpr auto push = conversion::string | pack::push;
  inline static constexpr auto store_into =
      conversion::string | pack::store_into<std::string>;
};

template <>
struct ActionFor<bool> {
  inline static constexpr auto set_once = conversion::boolean | pack::set_once;
  inline static constexpr auto push = conversion::boolean | pack::push;
  inline static constexpr auto store_into =
      conversion::boolean | pack::store_into<bool>;
};

template <>
struct ActionFor<std::filesystem::path> {
  inline static constexpr auto set_once = conversion::path | pack::set_once;
  inline static constexpr auto push = conversion::path | pack::push;
  inline static constexpr auto store_into =
      conversion::path | pack::store_into<std::filesystem::path>;
};

template <std::integral T>
struct ActionFor<T> {
  inline static constexpr auto set_once =
      conversion::integer<T> | pack::set_once;
  inline static constexpr auto push = conversion::integer<T> | pack::push;
  inline static constexpr auto store_into =
      conversion::integer<T> | pack::store_into<T>;
};

template <std::floating_point T>
struct ActionFor<T> {
  inline static constexpr auto set_once =
      conversion::floating<T> | pack::set_once;
  inline static constexpr auto push = conversion::floating<T> | pack::push;
  inline static constexpr auto store_into =
      conversion::floating<T> | pack::store_into<T>;
};

template <class T, Nargs N>
struct PositionalActionFor {
  inline static constexpr auto value = []() -> auto {
    if constexpr (N.max == 1) {
      return ActionFor<T>::set_once;
    } else {
      return ActionFor<T>::push;
    }
  }();
};

}  // namespace detail

// ── Core public API ───────────────────────────────────────────────────────────

/**
 * @brief A boolean flag option that stores `true` when present.
 *
 * Storage type: `bool`. Default: `false`.
 *
 * @tparam Name      Long option name (e.g. `"verbose"`).
 * @tparam ShortName Short option character (e.g. `'v'`), or `'\0'` for none.
 */
template <StringLiteral Name, char ShortName = '\0'>
using Flag = ArgImpl<Name, ShortName, nargs::none, pack::set_true>;

/**
 * @brief An option whose parsed value is written directly into an external
 * variable.
 *
 * The field stores a pointer `T*`; use the `bind(var)` method or the reference
 * constructor to point it at the target variable before parsing.
 *
 * Storage type: `T*`.
 *
 * @tparam T         The value type of the external variable.
 * @tparam Name      Long option name.
 * @tparam ShortName Short option character, or `'\0'`.
 */
template <class T, StringLiteral Name, char ShortName = '\0'>
using BoundOption =
    ArgImpl<Name, ShortName, nargs::one, detail::ActionFor<T>::store_into>;

/** @brief `BoundOption` specialisation for `std::string`. */
template <StringLiteral Name, char ShortName = '\0'>
using BoundStringOption = BoundOption<std::string, Name, ShortName>;

/** @brief `BoundOption` specialisation for `int`. */
template <StringLiteral Name, char ShortName = '\0'>
using BoundIntOption = BoundOption<int, Name, ShortName>;

/** @brief `BoundOption` specialisation for `double`. */
template <StringLiteral Name, char ShortName = '\0'>
using BoundDoubleOption = BoundOption<double, Name, ShortName>;

/** @brief `BoundOption` specialisation for `std::filesystem::path`. */
template <StringLiteral Name, char ShortName = '\0'>
using BoundPathOption = BoundOption<std::filesystem::path, Name, ShortName>;

/**
 * @brief A built-in help flag that prints help and exits with code 0.
 *
 * Default names: `--help` / `-h`. Override via template parameters.
 *
 * @tparam Name      Long option name. Default: `"help"`.
 * @tparam ShortName Short option character. Default: `'h'`.
 */
template <StringLiteral Name = "help", char ShortName = 'h'>
using Help = ArgImpl<Name, ShortName, nargs::none,
                     action::print_help | action::exit_success>;

/**
 * @brief A single-value option that stores the parsed result in
 * `std::optional<T>`.
 *
 * Storage type: `std::optional<T>`. Present when the option was supplied on
 * the command line, absent (nullopt) otherwise.
 *
 * @tparam T         The value type to parse (e.g. `int`, `std::string`).
 * @tparam Name      Long option name.
 * @tparam ShortName Short option character, or `'\0'`.
 */
template <class T, StringLiteral Name, char ShortName = '\0'>
using Option =
    ArgImpl<Name, ShortName, nargs::one, detail::ActionFor<T>::set_once>;

/**
 * @brief A multi-value option that appends each occurrence to a
 * `std::vector<T>`.
 *
 * Storage type: `std::vector<T>`.
 *
 * @tparam T         The element type.
 * @tparam Name      Long option name.
 * @tparam ShortName Short option character, or `'\0'`.
 * @tparam N         `Nargs` descriptor. Default: `nargs::one_or_more`.
 */
template <class T, StringLiteral Name, char ShortName = '\0',
          Nargs N = nargs::one_or_more>
using ListOption = ArgImpl<Name, ShortName, N, detail::ActionFor<T>::push>;

/**
 * @brief A typed positional argument.
 *
 * Storage type: `std::optional<T>` for `N.max == 1`, `std::vector<T>` for
 * multi-value.
 *
 * @tparam T The value type to parse.
 * @tparam N `Nargs` descriptor. Default: `nargs::one`.
 */
template <class T, Nargs N = nargs::one>
using Positional = PositionalImpl<N, detail::PositionalActionFor<T, N>::value>;

// ── Convenience aliases for common types ─────────────────────────────────────

/** @brief `Option<std::string, Name, ShortName>`. */
template <StringLiteral Name, char ShortName = '\0'>
using StringOption = Option<std::string, Name, ShortName>;

/** @brief `Option<int, Name, ShortName>`. */
template <StringLiteral Name, char ShortName = '\0'>
using IntOption = Option<int, Name, ShortName>;

/** @brief `Option<double, Name, ShortName>`. */
template <StringLiteral Name, char ShortName = '\0'>
using DoubleOption = Option<double, Name, ShortName>;

/** @brief `Option<std::filesystem::path, Name, ShortName>`. */
template <StringLiteral Name, char ShortName = '\0'>
using PathOption = Option<std::filesystem::path, Name, ShortName>;

}  // namespace cli
