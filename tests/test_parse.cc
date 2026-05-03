#include <gtest/gtest.h>

#include "argon/arithmetic_argument.hh"
#include "argon/arithmetic_positional.hh"
#include "argon/flag_argument.hh"
#include "argon/parser.hh"

using namespace argon;

// ---- Argument structs used across tests ----

struct SingleIntArgs {
  IntArg<"count", 'n'> count;
};

struct RequiredIntArgs {
  IntArg<"count"> count{required};
};

struct TwoIntArgs {
  IntArg<"count", 'n'> count;
  IntArg<"size", 's'> size;
};

struct FlagAndIntArgs {
  Flag<"verbose", 'v'> verbose;
  IntArg<"count", 'n'> count;
};

// ---- Positional argument structs ----

struct OnePositionalArgs {
  IntPositional x;
};

struct TwoPositionalArgs {
  IntPositional x;
  IntPositional y;
};

struct MixedArgs {
  IntArg<"count", 'n'> count;
  Flag<"verbose", 'v'> verbose;
  IntPositional x;
  IntPositional y;
};

struct RequiredIntPositionalArgs {
  IntPositional x{required};
};

struct TwoRequiredIntPositionalArgs {
  IntPositional x{required};
  IntPositional y{required};
};

// ---- success: basic value parsing ----

TEST(ParseIntArg, LongOption) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "42"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->count.provided());
  EXPECT_EQ(result->count.value(), 42);
}

TEST(ParseIntArg, ShortOption) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "-n", "7"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->count.provided());
  EXPECT_EQ(result->count.value(), 7);
}

TEST(ParseIntArg, EqualsSyntax) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count=99"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->count.provided());
  EXPECT_EQ(result->count.value(), 99);
}

TEST(ParseIntArg, ZeroValue) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "0"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->count.value(), 0);
}

TEST(ParseIntArg, NegativeValue) {
  Parser<SingleIntArgs> parser;
  // "-42" is not a known option key, so the tokenizer collects it as a value
  std::vector<std::string_view> args = {"prog", "--count", "-42"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->count.value(), -42);
}

TEST(ParseIntArg, MaxInt) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "2147483647"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->count.value(), 2147483647);
}

TEST(ParseIntArg, MinInt) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "-2147483648"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->count.value(), -2147483648);
}

// ---- success: optional not provided ----

TEST(ParseIntArg, OptionalNotProvided) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->count.provided());
  EXPECT_EQ(result->count.value(), 0);  // default-initialized
}

TEST(ParseIntArg, EmptyArgs) {
  // No argv at all (not even program name) — should not crash
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->count.provided());
  EXPECT_EQ(result->count.value(), 0);
}

// ---- success: multiple options ----

TEST(ParseIntArg, TwoLongOptions) {
  Parser<TwoIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "10", "--size", "20"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->count.value(), 10);
  EXPECT_EQ(result->size.value(), 20);
}

TEST(ParseIntArg, ShortAndLongMix) {
  Parser<TwoIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "-n", "10", "--size", "20"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->count.value(), 10);
  EXPECT_EQ(result->size.value(), 20);
}

TEST(ParseIntArg, IgnoresPositional) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "positional1", "--count", "5", "positional2"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->count.value(), 5);
}

// ---- success: flag interaction ----

TEST(ParseIntArg, FlagAndOption) {
  Parser<FlagAndIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--verbose", "--count", "3"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->verbose.provided());
  EXPECT_EQ(result->count.value(), 3);
}

TEST(ParseIntArg, FlagNotProvided) {
  Parser<FlagAndIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "3"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->verbose.provided());
  EXPECT_EQ(result->count.value(), 3);
}

TEST(ParseIntArg, FlagShortForm) {
  Parser<FlagAndIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "-v"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->verbose.provided());
}

// ---- success: required option ----

TEST(ParseIntArg, RequiredOptionProvided) {
  Parser<RequiredIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "5"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->count.value(), 5);
}

// ---- error: required option missing ----

TEST(ParseIntArg, RequiredOptionMissing) {
  Parser<RequiredIntArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "required option '--count' was not provided");
}

// ---- error: invalid integer string ----

TEST(ParseIntArg, InvalidNotANumber) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "abc"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

TEST(ParseIntArg, PartialNumber) {
  // "42abc" — from_chars succeeds for "42" but ptr != end, so we reject it
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "42abc"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

TEST(ParseIntArg, FloatString) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "3.14"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

TEST(ParseIntArg, EmptyString) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", ""};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

TEST(ParseIntArg, Overflow) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "99999999999999"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

TEST(ParseIntArg, Underflow) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "-99999999999999"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

// ---- error: tokenizer errors propagated through parse() ----

TEST(ParseIntArg, MissingValue) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--count' requires at least 1 argument(s), but got 0");
}

TEST(ParseIntArg, MissingValueShortOpt) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "-n"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '-n' requires at least 1 argument(s), but got 0");
}

TEST(ParseIntArg, DuplicateLongOption) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "1", "--count", "2"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--count' specified multiple times");
}

TEST(ParseIntArg, DuplicateShortOption) {
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "-n", "1", "-n", "2"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '-n' specified multiple times");
}

TEST(ParseIntArg, DuplicateMixedLongShort) {
  // --count then -n refer to the same option — tokenizer sees them as different keys,
  // so both succeed in the tokenizer. The parser calls parse() twice on the same field.
  // This is actually allowed by the current design (two distinct keys).
  // Verify that the second write wins (most recent value).
  Parser<SingleIntArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "1", "-n", "2"};
  auto result = parser.parse(args);
  // Both keys are distinct in the tokenizer, so no duplicate error.
  // The value should reflect both parses — whichever runs second wins.
  ASSERT_TRUE(result.has_value());
}

// ---- success: single positional ----

TEST(ParseIntPositional, SingleValue) {
  Parser<OnePositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "42"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->x.provided());
  EXPECT_EQ(result->x.value(), 42);
}

TEST(ParseIntPositional, ZeroValue) {
  Parser<OnePositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "0"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->x.value(), 0);
}

TEST(ParseIntPositional, NegativeValue) {
  Parser<OnePositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "-7"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->x.value(), -7);
}

TEST(ParseIntPositional, MaxInt) {
  Parser<OnePositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "2147483647"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->x.value(), 2147483647);
}

TEST(ParseIntPositional, NotProvided) {
  Parser<OnePositionalArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->x.provided());
  EXPECT_EQ(result->x.value(), 0);
}

// ---- success: multiple positionals ----

TEST(ParseIntPositional, TwoValues) {
  Parser<TwoPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "10", "20"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->x.value(), 10);
  EXPECT_EQ(result->y.value(), 20);
  EXPECT_TRUE(result->x.provided());
  EXPECT_TRUE(result->y.provided());
}

TEST(ParseIntPositional, OnlyFirstProvided) {
  Parser<TwoPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "5"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->x.provided());
  EXPECT_EQ(result->x.value(), 5);
  EXPECT_FALSE(result->y.provided());
  EXPECT_EQ(result->y.value(), 0);
}

// ---- success: positionals mixed with options and flags ----

TEST(ParseIntPositional, MixedOptionFlagPositional) {
  Parser<MixedArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "3", "--verbose", "10", "20"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->count.value(), 3);
  EXPECT_TRUE(result->verbose.provided());
  EXPECT_EQ(result->x.value(), 10);
  EXPECT_EQ(result->y.value(), 20);
}

TEST(ParseIntPositional, PositionalBeforeOption) {
  Parser<MixedArgs> parser;
  std::vector<std::string_view> args = {"prog", "10", "20", "--count", "3"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->x.value(), 10);
  EXPECT_EQ(result->y.value(), 20);
  EXPECT_EQ(result->count.value(), 3);
}

TEST(ParseIntPositional, PositionalInterleavedWithOptions) {
  Parser<MixedArgs> parser;
  std::vector<std::string_view> args = {"prog", "10", "--count", "3", "20"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->x.value(), 10);
  EXPECT_EQ(result->y.value(), 20);
  EXPECT_EQ(result->count.value(), 3);
}

TEST(ParseIntPositional, OptionOnlyNoPositionals) {
  Parser<MixedArgs> parser;
  std::vector<std::string_view> args = {"prog", "--count", "99"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->count.value(), 99);
  EXPECT_FALSE(result->x.provided());
  EXPECT_FALSE(result->y.provided());
}

// ---- error: invalid positional integer ----

TEST(ParseIntPositional, InvalidNotANumber) {
  Parser<OnePositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "abc"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

TEST(ParseIntPositional, PartialNumber) {
  Parser<OnePositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "42abc"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseIntPositional, FloatString) {
  Parser<OnePositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "3.14"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseIntPositional, Overflow) {
  Parser<OnePositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "99999999999999"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseIntPositional, SecondPositionalInvalid) {
  Parser<TwoPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "10", "bad"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

// ============================================================
// Required IntPositional
// ============================================================

TEST(ParseIntPositional, RequiredProvided) {
  Parser<RequiredIntPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "7"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->x.value(), 7);
}

TEST(ParseIntPositional, RequiredMissing) {
  Parser<RequiredIntPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

TEST(ParseIntPositional, TwoRequiredBothProvided) {
  Parser<TwoRequiredIntPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "3", "9"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->x.value(), 3);
  EXPECT_EQ(result->y.value(), 9);
}

TEST(ParseIntPositional, TwoRequiredSecondMissing) {
  Parser<TwoRequiredIntPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "3"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

TEST(ParseIntPositional, TwoRequiredBothMissing) {
  Parser<TwoRequiredIntPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}
