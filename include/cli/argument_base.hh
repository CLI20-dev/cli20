#pragma once

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
 * The default `{0, -1}` is equivalent to `nargs::zero_or_more`.
 */
struct Nargs {
  int min = 0;   ///< Minimum number of values (>= 0).
  int max = -1;  ///< Maximum number of values; -1 means unlimited.
};

/**
 * @brief Controls whether an option or positional argument is mandatory.
 */
enum class Presence { required, optional };

/** @brief Convenience constant for `Presence::required`. */
inline constexpr auto required = Presence::required;

/** @brief Convenience constant for `Presence::optional`. */
inline constexpr auto optional = Presence::optional;

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

template <int N>
inline constexpr Nargs exactly{.min = N, .max = N};

template <int Min, int Max>
inline constexpr Nargs between{.min = Min, .max = Max};

}  // namespace nargs

}  // namespace cli
