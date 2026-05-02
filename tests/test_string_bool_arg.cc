#include <gtest/gtest.h>

#include "argon/bool_argument.hh"
#include "argon/parser.hh"
#include "argon/string_argument.hh"

using namespace argon;

// ---- Argument structs ----

struct SingleStrArgs {
  StrArg<"name", 'n'> name;
};

struct RequiredStrArgs {
  StrArg<"name"> name{required};
};

struct StrListArgs {
  StrListArg<"names", 'n'> names{nargs::one_or_more};
};

struct TwoStrPositionalArgs {
  StrPositional src;
  StrPositional dst;
};

struct MixedStrArgs {
  StrArg<"output", 'o'> output;
  Flag<"verbose", 'v'> verbose;
  StrPositional input;
};

struct SingleBoolArgs {
  BoolArg<"flag", 'f'> flag;
};

struct RequiredBoolArgs {
  BoolArg<"flag"> flag{required};
};

struct BoolPositionalArgs {
  BoolPositional enabled;
};

struct MixedBoolStrArgs {
  BoolArg<"debug", 'd'> debug;
  StrArg<"level", 'l'> level;
  BoolPositional active;
};

struct BoolListArgArgs {
  BoolListArg<"flags", 'f'> flags{nargs::one_or_more};
};

// ============================================================
// StringArg tests
// ============================================================

// ---- success: basic parsing ----

TEST(ParseStrArg, LongOption) {
  Parser<SingleStrArgs> parser;
  std::vector<std::string_view> args = {"prog", "--name", "hello"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->name.provided());
  EXPECT_EQ(result->name.value(), "hello");
}

TEST(ParseStrArg, ShortOption) {
  Parser<SingleStrArgs> parser;
  std::vector<std::string_view> args = {"prog", "-n", "world"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->name.provided());
  EXPECT_EQ(result->name.value(), "world");
}

TEST(ParseStrArg, EqualsSyntax) {
  Parser<SingleStrArgs> parser;
  std::vector<std::string_view> args = {"prog", "--name=foo"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->name.value(), "foo");
}

TEST(ParseStrArg, EqualsSyntaxValueContainsEquals) {
  Parser<SingleStrArgs> parser;
  std::vector<std::string_view> args = {"prog", "--name=a=b"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->name.value(), "a=b");
}

TEST(ParseStrArg, EmptyStringValue) {
  // Empty string is valid for string args
  Parser<SingleStrArgs> parser;
  std::vector<std::string_view> args = {"prog", "--name", ""};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->name.provided());
  EXPECT_EQ(result->name.value(), "");
}

TEST(ParseStrArg, ValueWithSpacesViaEqualsNotPossible) {
  // Value containing a hyphen prefix is fine (not mistaken for an option)
  Parser<SingleStrArgs> parser;
  std::vector<std::string_view> args = {"prog", "--name", "-not-an-option"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->name.value(), "-not-an-option");
}

// ---- success: optional not provided ----

TEST(ParseStrArg, OptionalNotProvided) {
  Parser<SingleStrArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->name.provided());
  EXPECT_EQ(result->name.value(), "");
}

// ---- success: required provided ----

TEST(ParseStrArg, RequiredProvided) {
  Parser<RequiredStrArgs> parser;
  std::vector<std::string_view> args = {"prog", "--name", "argon"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->name.value(), "argon");
}

// ---- error: required missing ----

TEST(ParseStrArg, RequiredMissing) {
  Parser<RequiredStrArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "required option '--name' was not provided");
}

// ---- error: tokenizer errors ----

TEST(ParseStrArg, MissingValue) {
  Parser<SingleStrArgs> parser;
  std::vector<std::string_view> args = {"prog", "--name"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--name' requires at least 1 argument(s), but got 0");
}

TEST(ParseStrArg, DuplicateLongOption) {
  Parser<SingleStrArgs> parser;
  std::vector<std::string_view> args = {"prog", "--name", "a", "--name", "b"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--name' specified multiple times");
}

// ============================================================
// StringListArg tests
// ============================================================

TEST(ParseStrListArg, SingleValue) {
  Parser<StrListArgs> parser;
  std::vector<std::string_view> args = {"prog", "--names", "alice"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->names.provided());
  ASSERT_EQ(result->names.value().size(), 1u);
  EXPECT_EQ(result->names.value()[0], "alice");
}

TEST(ParseStrListArg, MultipleValues) {
  Parser<StrListArgs> parser;
  std::vector<std::string_view> args = {"prog", "--names", "alice", "bob", "charlie"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->names.value().size(), 3u);
  EXPECT_EQ(result->names.value()[0], "alice");
  EXPECT_EQ(result->names.value()[1], "bob");
  EXPECT_EQ(result->names.value()[2], "charlie");
}

TEST(ParseStrListArg, ShortOption) {
  Parser<StrListArgs> parser;
  std::vector<std::string_view> args = {"prog", "-n", "x", "y"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->names.value().size(), 2u);
}

TEST(ParseStrListArg, NotProvided) {
  Parser<StrListArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->names.provided());
  EXPECT_TRUE(result->names.value().empty());
}

TEST(ParseStrListArg, MissingValue) {
  Parser<StrListArgs> parser;
  std::vector<std::string_view> args = {"prog", "--names"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

// ============================================================
// StringPositional tests
// ============================================================

TEST(ParseStrPositional, SingleValue) {
  Parser<TwoStrPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "src.txt"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->src.provided());
  EXPECT_EQ(result->src.value(), "src.txt");
  EXPECT_FALSE(result->dst.provided());
}

TEST(ParseStrPositional, TwoValues) {
  Parser<TwoStrPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "src.txt", "dst.txt"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->src.value(), "src.txt");
  EXPECT_EQ(result->dst.value(), "dst.txt");
}

TEST(ParseStrPositional, EmptyValue) {
  Parser<TwoStrPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", ""};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->src.provided());
  EXPECT_EQ(result->src.value(), "");
}

TEST(ParseStrPositional, NotProvided) {
  Parser<TwoStrPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->src.provided());
  EXPECT_FALSE(result->dst.provided());
}

TEST(ParseStrPositional, MixedWithOption) {
  Parser<MixedStrArgs> parser;
  std::vector<std::string_view> args = {"prog", "--output", "out.txt", "--verbose", "in.txt"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->output.value(), "out.txt");
  EXPECT_TRUE(result->verbose.provided());
  EXPECT_EQ(result->input.value(), "in.txt");
}

TEST(ParseStrPositional, PositionalBeforeOption) {
  Parser<MixedStrArgs> parser;
  std::vector<std::string_view> args = {"prog", "in.txt", "--output", "out.txt"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->input.value(), "in.txt");
  EXPECT_EQ(result->output.value(), "out.txt");
}

// ============================================================
// BoolArg tests
// ============================================================

// ---- success ----

TEST(ParseBoolArg, TrueLongOption) {
  Parser<SingleBoolArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flag", "true"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->flag.provided());
  EXPECT_EQ(result->flag.value(), true);
}

TEST(ParseBoolArg, FalseLongOption) {
  Parser<SingleBoolArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flag", "false"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->flag.provided());
  EXPECT_EQ(result->flag.value(), false);
}

TEST(ParseBoolArg, TrueShortOption) {
  Parser<SingleBoolArgs> parser;
  std::vector<std::string_view> args = {"prog", "-f", "true"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->flag.value(), true);
}

TEST(ParseBoolArg, FalseShortOption) {
  Parser<SingleBoolArgs> parser;
  std::vector<std::string_view> args = {"prog", "-f", "false"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->flag.value(), false);
}

TEST(ParseBoolArg, EqualsSyntaxTrue) {
  Parser<SingleBoolArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flag=true"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->flag.value(), true);
}

TEST(ParseBoolArg, EqualsSyntaxFalse) {
  Parser<SingleBoolArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flag=false"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->flag.value(), false);
}

TEST(ParseBoolArg, OptionalNotProvided) {
  Parser<SingleBoolArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->flag.provided());
  EXPECT_EQ(result->flag.value(), false);
}

TEST(ParseBoolArg, RequiredProvided) {
  Parser<RequiredBoolArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flag", "true"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->flag.value(), true);
}

// ---- error ----

TEST(ParseBoolArg, RequiredMissing) {
  Parser<RequiredBoolArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "required option '--flag' was not provided");
}

TEST(ParseBoolArg, InvalidYes) {
  Parser<SingleBoolArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flag", "yes"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

TEST(ParseBoolArg, InvalidOne) {
  Parser<SingleBoolArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flag", "1"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseBoolArg, InvalidZero) {
  Parser<SingleBoolArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flag", "0"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseBoolArg, InvalidUppercaseTrue) {
  // Parsing is case-sensitive: "True" is not valid
  Parser<SingleBoolArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flag", "True"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseBoolArg, InvalidUppercaseFalse) {
  Parser<SingleBoolArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flag", "False"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseBoolArg, InvalidEmpty) {
  Parser<SingleBoolArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flag", ""};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseBoolArg, MissingValue) {
  Parser<SingleBoolArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flag"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--flag' requires at least 1 argument(s), but got 0");
}

TEST(ParseBoolArg, DuplicateOption) {
  Parser<SingleBoolArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flag", "true", "--flag", "false"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--flag' specified multiple times");
}

// ============================================================
// BoolPositional tests
// ============================================================

TEST(ParseBoolPositional, TrueValue) {
  Parser<BoolPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "true"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->enabled.provided());
  EXPECT_EQ(result->enabled.value(), true);
}

TEST(ParseBoolPositional, FalseValue) {
  Parser<BoolPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "false"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->enabled.provided());
  EXPECT_EQ(result->enabled.value(), false);
}

TEST(ParseBoolPositional, NotProvided) {
  Parser<BoolPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->enabled.provided());
  EXPECT_EQ(result->enabled.value(), false);
}

TEST(ParseBoolPositional, InvalidValue) {
  Parser<BoolPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "yes"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

TEST(ParseBoolPositional, InvalidNumeric) {
  Parser<BoolPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "1"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

TEST(ParseBoolPositional, CaseSensitiveTrue) {
  Parser<BoolPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "TRUE"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}

// ---- mixed bool+str ----

TEST(ParseMixedBoolStr, AllProvided) {
  Parser<MixedBoolStrArgs> parser;
  std::vector<std::string_view> args = {"prog", "--debug", "true", "--level", "info", "true"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->debug.value(), true);
  EXPECT_EQ(result->level.value(), "info");
  EXPECT_EQ(result->active.value(), true);
}

TEST(ParseMixedBoolStr, OnlyOptions) {
  Parser<MixedBoolStrArgs> parser;
  std::vector<std::string_view> args = {"prog", "--debug", "false", "--level", "warn"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->debug.value(), false);
  EXPECT_EQ(result->level.value(), "warn");
  EXPECT_FALSE(result->active.provided());
}

// ============================================================
// BoolListArg tests
// ============================================================

TEST(ParseBoolListArg, SingleTrue) {
  Parser<BoolListArgArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flags", "true"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->flags.provided());
  ASSERT_EQ(result->flags.value().size(), 1u);
  EXPECT_EQ(result->flags.value()[0], true);
}

TEST(ParseBoolListArg, MultipleValues) {
  Parser<BoolListArgArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flags", "true", "false", "true"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->flags.value().size(), 3u);
  EXPECT_EQ(result->flags.value()[0], true);
  EXPECT_EQ(result->flags.value()[1], false);
  EXPECT_EQ(result->flags.value()[2], true);
}

TEST(ParseBoolListArg, ShortOption) {
  Parser<BoolListArgArgs> parser;
  std::vector<std::string_view> args = {"prog", "-f", "false", "true"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->flags.value().size(), 2u);
  EXPECT_EQ(result->flags.value()[0], false);
  EXPECT_EQ(result->flags.value()[1], true);
}

TEST(ParseBoolListArg, NotProvided) {
  Parser<BoolListArgArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->flags.provided());
  EXPECT_TRUE(result->flags.value().empty());
}

TEST(ParseBoolListArg, MissingValue) {
  Parser<BoolListArgArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flags"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

TEST(ParseBoolListArg, InvalidValueInList) {
  Parser<BoolListArgArgs> parser;
  std::vector<std::string_view> args = {"prog", "--flags", "true", "yes"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
}
