#include <gtest/gtest.h>

#include "argon/float_argument.hh"
#include "argon/float_positional.hh"
#include "argon/parser.hh"

using namespace argon;

// ---- Argument structs ----

struct SingleFloatArgs {
  FloatArg<"ratio", 'r'> ratio;
};

struct RequiredFloatArgs {
  FloatArg<"ratio"> ratio{required};
};

struct SingleDoubleArgs {
  DoubleArg<"value", 'v'> value;
};

struct RequiredDoubleArgs {
  DoubleArg<"value"> value{required};
};

struct FloatListArgs {
  FloatListArg<"values", 'v'> values{nargs::one_or_more};
};

struct DoubleListArgs {
  DoubleListArg<"values", 'v'> values{nargs::one_or_more};
};

struct MixedFloatArgs {
  FloatArg<"alpha", 'a'> alpha;
  DoubleArg<"beta", 'b'> beta;
  FloatPositional x;
  DoublePositional y;
};

struct SingleFloatPositionalArgs {
  FloatPositional x;
};

struct SingleDoublePositionalArgs {
  DoublePositional x;
};

// ============================================================
// FloatArg tests
// ============================================================

// ---- success: basic parsing ----

TEST(ParseFloatArg, IntegerString) {
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--ratio", "42"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->ratio.seen());
  EXPECT_FLOAT_EQ(result->ratio.value(), 42.0f);
}

TEST(ParseFloatArg, DecimalValue) {
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--ratio", "3.14"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->ratio.value(), 3.14f);
}

TEST(ParseFloatArg, NegativeValue) {
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--ratio", "-1.5"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->ratio.value(), -1.5f);
}

TEST(ParseFloatArg, ZeroValue) {
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--ratio", "0"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->ratio.value(), 0.0f);
}

TEST(ParseFloatArg, ScientificNotation) {
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--ratio", "1.5e2"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->ratio.value(), 150.0f);
}

TEST(ParseFloatArg, NegativeScientific) {
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--ratio", "-2.5e-1"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->ratio.value(), -0.25f);
}

TEST(ParseFloatArg, ExactHalf) {
  // 0.5 is exactly representable in binary float
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--ratio", "0.5"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->ratio.value(), 0.5f);
}

TEST(ParseFloatArg, ShortOption) {
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "-r", "2.5"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->ratio.value(), 2.5f);
}

TEST(ParseFloatArg, EqualsSyntax) {
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--ratio=1.25"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->ratio.value(), 1.25f);
}

TEST(ParseFloatArg, OptionalNotProvided) {
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->ratio.seen());
  EXPECT_EQ(result->ratio.value(), 0.0f);
}

// ---- success: required ----

TEST(ParseFloatArg, RequiredProvided) {
  Parser<RequiredFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--ratio", "1.0"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->ratio.value(), 1.0f);
}

// ---- error: required missing ----

TEST(ParseFloatArg, RequiredMissing) {
  Parser<RequiredFloatArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "required option '--ratio' was not provided");
}

// ---- error: invalid input ----

TEST(ParseFloatArg, NotANumber) {
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--ratio", "abc"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

TEST(ParseFloatArg, PartialNumber) {
  // from_chars parses "3.14" but ptr stops at 'x', so we reject it
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--ratio", "3.14x"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseFloatArg, EmptyString) {
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--ratio", ""};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseFloatArg, FloatOverflow) {
  // 1e39 exceeds float max (~3.4e38)
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--ratio", "1e39"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

// ---- error: tokenizer errors propagated ----

TEST(ParseFloatArg, MissingValue) {
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--ratio"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--ratio' requires at least 1 argument(s), but got 0");
}

TEST(ParseFloatArg, DuplicateOption) {
  Parser<SingleFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--ratio", "1.0", "--ratio", "2.0"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--ratio' specified multiple times");
}

// ---- FloatListArg ----

TEST(ParseFloatArg, ListTwoValues) {
  Parser<FloatListArgs> parser;
  std::vector<std::string_view> args = {"prog", "--values", "1.0", "2.5", "3.0"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->values.value().size(), 3u);
  EXPECT_FLOAT_EQ(result->values.value()[0], 1.0f);
  EXPECT_FLOAT_EQ(result->values.value()[1], 2.5f);
  EXPECT_FLOAT_EQ(result->values.value()[2], 3.0f);
}

TEST(ParseFloatArg, ListInvalidElement) {
  Parser<FloatListArgs> parser;
  std::vector<std::string_view> args = {"prog", "--values", "1.0", "bad", "3.0"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

// ============================================================
// DoubleArg tests
// ============================================================

TEST(ParseDoubleArg, DecimalValue) {
  Parser<SingleDoubleArgs> parser;
  std::vector<std::string_view> args = {"prog", "--value", "3.141592653589793"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_DOUBLE_EQ(result->value.value(), 3.141592653589793);
}

TEST(ParseDoubleArg, ScientificNotation) {
  Parser<SingleDoubleArgs> parser;
  std::vector<std::string_view> args = {"prog", "--value", "6.022e23"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_DOUBLE_EQ(result->value.value(), 6.022e23);
}

TEST(ParseDoubleArg, NegativeValue) {
  Parser<SingleDoubleArgs> parser;
  std::vector<std::string_view> args = {"prog", "--value", "-2.718281828"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_DOUBLE_EQ(result->value.value(), -2.718281828);
}

TEST(ParseDoubleArg, ZeroValue) {
  Parser<SingleDoubleArgs> parser;
  std::vector<std::string_view> args = {"prog", "--value", "0.0"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->value.value(), 0.0);
}

TEST(ParseDoubleArg, ShortOption) {
  Parser<SingleDoubleArgs> parser;
  std::vector<std::string_view> args = {"prog", "-v", "1.0"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_DOUBLE_EQ(result->value.value(), 1.0);
}

TEST(ParseDoubleArg, EqualsSyntax) {
  Parser<SingleDoubleArgs> parser;
  std::vector<std::string_view> args = {"prog", "--value=0.125"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->value.value(), 0.125);  // 0.125 is exact in binary
}

TEST(ParseDoubleArg, OptionalNotProvided) {
  Parser<SingleDoubleArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->value.seen());
  EXPECT_EQ(result->value.value(), 0.0);
}

TEST(ParseDoubleArg, RequiredMissing) {
  Parser<RequiredDoubleArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "required option '--value' was not provided");
}

TEST(ParseDoubleArg, DoubleOverflow) {
  // 1e309 exceeds double max (~1.8e308)
  Parser<SingleDoubleArgs> parser;
  std::vector<std::string_view> args = {"prog", "--value", "1e309"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseDoubleArg, PartialNumber) {
  Parser<SingleDoubleArgs> parser;
  std::vector<std::string_view> args = {"prog", "--value", "1.0abc"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseDoubleArg, DoubleListValues) {
  Parser<DoubleListArgs> parser;
  std::vector<std::string_view> args = {"prog", "--values", "1.1", "2.2"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->values.value().size(), 2u);
  EXPECT_DOUBLE_EQ(result->values.value()[0], 1.1);
  EXPECT_DOUBLE_EQ(result->values.value()[1], 2.2);
}

// ============================================================
// FloatPositional tests
// ============================================================

TEST(ParseFloatPositional, SingleValue) {
  Parser<SingleFloatPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "3.14"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->x.seen());
  EXPECT_FLOAT_EQ(result->x.value(), 3.14f);
}

TEST(ParseFloatPositional, IntegerString) {
  Parser<SingleFloatPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "42"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->x.value(), 42.0f);
}

TEST(ParseFloatPositional, NegativeValue) {
  Parser<SingleFloatPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "-0.5"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->x.value(), -0.5f);
}

TEST(ParseFloatPositional, NotProvided) {
  Parser<SingleFloatPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->x.seen());
}

TEST(ParseFloatPositional, InvalidNotANumber) {
  Parser<SingleFloatPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "abc"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseFloatPositional, PartialNumber) {
  Parser<SingleFloatPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "1.5z"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseFloatPositional, Overflow) {
  Parser<SingleFloatPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "1e39"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

// ============================================================
// DoublePositional tests
// ============================================================

TEST(ParseDoublePositional, SingleValue) {
  Parser<SingleDoublePositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "2.718281828459045"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->x.seen());
  EXPECT_DOUBLE_EQ(result->x.value(), 2.718281828459045);
}

TEST(ParseDoublePositional, NegativeValue) {
  Parser<SingleDoublePositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "-1.0"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->x.value(), -1.0);
}

TEST(ParseDoublePositional, NotProvided) {
  Parser<SingleDoublePositionalArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->x.seen());
}

TEST(ParseDoublePositional, InvalidNotANumber) {
  Parser<SingleDoublePositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "nope"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseDoublePositional, Overflow) {
  Parser<SingleDoublePositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "1e309"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

// ============================================================
// Mixed float/double args + positionals
// ============================================================

TEST(ParseMixedFloat, AllProvided) {
  Parser<MixedFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "--alpha", "0.5", "--beta", "1.5", "2.5", "3.5"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->alpha.value(), 0.5f);
  EXPECT_DOUBLE_EQ(result->beta.value(), 1.5);
  EXPECT_FLOAT_EQ(result->x.value(), 2.5f);
  EXPECT_DOUBLE_EQ(result->y.value(), 3.5);
}

TEST(ParseMixedFloat, PositionalBeforeOptions) {
  Parser<MixedFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "10.0", "20.0", "--alpha", "0.1", "--beta", "0.2"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->x.value(), 10.0f);
  EXPECT_DOUBLE_EQ(result->y.value(), 20.0);
  EXPECT_FLOAT_EQ(result->alpha.value(), 0.1f);
  EXPECT_DOUBLE_EQ(result->beta.value(), 0.2);
}

TEST(ParseMixedFloat, OnlyPositionals) {
  Parser<MixedFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "1.0", "2.0"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->x.value(), 1.0f);
  EXPECT_DOUBLE_EQ(result->y.value(), 2.0);
  EXPECT_FALSE(result->alpha.seen());
  EXPECT_FALSE(result->beta.seen());
}

TEST(ParseMixedFloat, InvalidPositional) {
  Parser<MixedFloatArgs> parser;
  std::vector<std::string_view> args = {"prog", "1.0", "bad"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}
