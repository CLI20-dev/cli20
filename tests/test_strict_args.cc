#include <gtest/gtest.h>

#include "argon/arithmetic_argument.hh"
#include "argon/bool_argument.hh"
#include "argon/flag_argument.hh"
#include "argon/parser.hh"
#include "argon/string_argument.hh"

using namespace argon;

// ---- Argument structs ----

struct NoArgs {};

struct OnlyOption {
  IntArg<"count", 'n'> count;
};

struct OnlyFlag {
  FlagArg<"verbose", 'v'> verbose;
};

struct OnePositional {
  IntPositional x;
};

struct TwoPositionals {
  IntPositional x;
  IntPositional y;
};

struct OptionAndPositional {
  IntArg<"count", 'n'> count;
  IntPositional x;
};

// ============================================================
// Unknown option / flag
// ============================================================

TEST(UnknownOption, LongOptionIsError) {
  Parser<OnlyOption> parser;
  std::vector<std::string_view> args = {"prog", "--unknown"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, ErrorCode::unknown_option);
  EXPECT_EQ(result.error().subject, "--unknown");
}

TEST(UnknownOption, ShortOptionIsError) {
  Parser<OnlyOption> parser;
  std::vector<std::string_view> args = {"prog", "-x"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, ErrorCode::unknown_option);
  EXPECT_EQ(result.error().subject, "-x");
}

TEST(UnknownOption, LongOptionWithEqualsSyntaxIsError) {
  Parser<OnlyOption> parser;
  std::vector<std::string_view> args = {"prog", "--unknown=value"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, ErrorCode::unknown_option);
  EXPECT_EQ(result.error().subject, "--unknown");
}

TEST(UnknownOption, KnownFlagWithEqualsSyntaxIsError) {
  // '=' syntax is not supported for flags (nargs=0).
  Parser<OnlyFlag> parser;
  std::vector<std::string_view> args = {"prog", "--verbose=true"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, ErrorCode::unknown_option);
  EXPECT_EQ(result.error().subject, "--verbose");
}

TEST(UnknownOption, KnownOptionWithEqualsSyntaxWorksForNargsOne) {
  Parser<OnlyOption> parser;
  std::vector<std::string_view> args = {"prog", "--count=42"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->count.value(), 42);
}

TEST(UnknownOption, UnknownOptionAfterKnownOptionIsError) {
  Parser<OnlyOption> parser;
  std::vector<std::string_view> args = {"prog", "--count", "5", "--extra"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, ErrorCode::unknown_option);
  EXPECT_EQ(result.error().subject, "--extra");
}

TEST(UnknownOption, SingleDashAloneIsPositional) {
  // A bare '-' (commonly meaning stdin) is treated as a positional bare word,
  // not as an option prefix.
  Parser<OnePositional> parser;
  // '-' alone can't parse as int, but it should not error as unknown_option.
  std::vector<std::string_view> args = {"prog", "-"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  // Error is a conversion error, not unknown_option.
  EXPECT_NE(result.error().code, ErrorCode::unknown_option);
}

// ============================================================
// Positional argument overflow
// ============================================================

TEST(PositionalOverflow, NoPositionalFieldsRejectsBarePosArg) {
  Parser<OnlyOption> parser;
  std::vector<std::string_view> args = {"prog", "extra"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, ErrorCode::unexpected_argument);
  EXPECT_EQ(result.error().subject, "extra");
}

TEST(PositionalOverflow, OneFieldRejectsTwoValues) {
  Parser<OnePositional> parser;
  std::vector<std::string_view> args = {"prog", "1", "2"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, ErrorCode::unexpected_argument);
  EXPECT_EQ(result.error().subject, "2");
}

TEST(PositionalOverflow, TwoFieldsAcceptTwoValues) {
  Parser<TwoPositionals> parser;
  std::vector<std::string_view> args = {"prog", "10", "20"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->x.value(), 10);
  EXPECT_EQ(result->y.value(), 20);
}

TEST(PositionalOverflow, TwoFieldsRejectThreeValues) {
  Parser<TwoPositionals> parser;
  std::vector<std::string_view> args = {"prog", "10", "20", "30"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, ErrorCode::unexpected_argument);
  EXPECT_EQ(result.error().subject, "30");
}

TEST(PositionalOverflow, MixedOptionAndPositionalOverflow) {
  Parser<OptionAndPositional> parser;
  std::vector<std::string_view> args = {"prog", "--count", "5", "pos1", "pos2"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, ErrorCode::unexpected_argument);
  EXPECT_EQ(result.error().subject, "pos2");
}

// ============================================================
// '--' end-of-options separator
// ============================================================

TEST(DashDashSeparator, PassesDashPrefixedStringAsPositional) {
  // '--' allows '--foo' to be treated as a positional, not an unknown option.
  Parser<OnePositional> parser;
  // Can't actually parse "--foo" as int, but the important thing is it's NOT
  // an unknown_option error — it reaches the conversion stage.
  std::vector<std::string_view> args = {"prog", "--", "--foo"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().code, ErrorCode::unknown_option);
}

TEST(DashDashSeparator, PassesNegativeNumberAsPositional) {
  Parser<OnePositional> parser;
  std::vector<std::string_view> args = {"prog", "--", "-42"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->x.value(), -42);
}

TEST(DashDashSeparator, MultipleValuesAfterSeparator) {
  Parser<TwoPositionals> parser;
  std::vector<std::string_view> args = {"prog", "--", "-1", "-2"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->x.value(), -1);
  EXPECT_EQ(result->y.value(), -2);
}

TEST(DashDashSeparator, OptionsBeforeSeparatorStillWork) {
  Parser<OptionAndPositional> parser;
  std::vector<std::string_view> args = {"prog", "--count", "7", "--", "-99"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->count.value(), 7);
  EXPECT_EQ(result->x.value(), -99);
}

TEST(DashDashSeparator, SeparatorAloneIsOk) {
  Parser<OnlyOption> parser;
  std::vector<std::string_view> args = {"prog", "--count", "3", "--"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->count.value(), 3);
}

TEST(DashDashSeparator, DoubleDashOptionAsPositionalOverflowIsError) {
  // '--' allows '--verbose' as a positional, but overflow is still an error.
  Parser<OnlyOption> parser;
  std::vector<std::string_view> args = {"prog", "--", "--verbose"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, ErrorCode::unexpected_argument);
  EXPECT_EQ(result.error().subject, "--verbose");
}
