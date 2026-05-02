#include <gtest/gtest.h>

#include "argon/parser.hh"

using argon::detail::Nargs;
using argon::detail::tokenize;
using argon::nargs::exactly;
using argon::nargs::none;
using argon::nargs::one;
using argon::nargs::one_or_more;
using argon::nargs::zero_or_more;

using SpecMap = std::unordered_map<std::string, Nargs>;

// ---- success: positional ----

TEST(Tokenize, EmptyArgs) {
  SpecMap spec{{"--foo", one}};
  auto result = tokenize({}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->named.empty());
  EXPECT_TRUE(result->positional.empty());
}

TEST(Tokenize, AllPositional) {
  SpecMap spec{};
  auto result = tokenize({"a", "b", "c"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->named.empty());
  ASSERT_EQ(result->positional.size(), 3u);
}

TEST(Tokenize, UnknownArgGoesToPositional) {
  SpecMap spec{{"--foo", one}};
  auto result = tokenize({"unknown"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->named.empty());
  ASSERT_EQ(result->positional.size(), 1u);
  EXPECT_EQ(result->positional[0], "unknown");
}

// ---- success: flags / options ----

TEST(Tokenize, FlagNoValue) {
  SpecMap spec{{"--verbose", none}};
  auto result = tokenize({"--verbose"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->positional.empty());
  EXPECT_TRUE(result->named.at("--verbose").empty());
}

TEST(Tokenize, OptionOneValue) {
  SpecMap spec{{"--output", one}};
  auto result = tokenize({"--output", "file.txt"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->positional.empty());
  EXPECT_EQ(result->named.at("--output")[0], "file.txt");
}

TEST(Tokenize, ShortOption) {
  SpecMap spec{{"-o", one}};
  auto result = tokenize({"-o", "out.txt"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->positional.empty());
  EXPECT_EQ(result->named.at("-o")[0], "out.txt");
}

TEST(Tokenize, ExactlyTwoValues) {
  SpecMap spec{{"--range", exactly<2>}};
  auto result = tokenize({"--range", "1", "2"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->positional.empty());
  ASSERT_EQ(result->named.at("--range").size(), 2u);
  EXPECT_EQ(result->named.at("--range")[0], "1");
  EXPECT_EQ(result->named.at("--range")[1], "2");
}

TEST(Tokenize, ZeroOrMoreWithNoValues) {
  SpecMap spec{{"--list", zero_or_more}, {"--end", none}};
  auto result = tokenize({"--list", "--end"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->positional.empty());
  EXPECT_TRUE(result->named.at("--list").empty());
  EXPECT_TRUE(result->named.contains("--end"));
}

TEST(Tokenize, ZeroOrMoreAtEndOfArgs) {
  SpecMap spec{{"--list", zero_or_more}};
  auto result = tokenize({"--list"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->positional.empty());
  EXPECT_TRUE(result->named.at("--list").empty());
}

TEST(Tokenize, VariadicCollectsUntilNextOption) {
  SpecMap spec{{"--list", zero_or_more}, {"--end", none}};
  auto result = tokenize({"--list", "a", "b", "c", "--end"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->positional.empty());
  ASSERT_EQ(result->named.at("--list").size(), 3u);
  EXPECT_TRUE(result->named.contains("--end"));
}

TEST(Tokenize, VariadicCollectsUntilEnd) {
  SpecMap spec{{"--list", one_or_more}};
  auto result = tokenize({"--list", "x", "y"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->positional.empty());
  ASSERT_EQ(result->named.at("--list").size(), 2u);
}

TEST(Tokenize, CommandName) {
  SpecMap spec{{"sub", {.min = 0, .max = std::nullopt}}};
  auto result = tokenize({"sub"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->positional.empty());
  EXPECT_TRUE(result->named.contains("sub"));
}

TEST(Tokenize, MixedArgs) {
  SpecMap spec{{"--output", one}, {"--verbose", none}, {"-n", one}};
  auto result =
      tokenize({"pos1", "--output", "file.txt", "--verbose", "-n", "42", "pos2"}, spec);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->positional.size(), 2u);
  EXPECT_EQ(result->positional[0], "pos1");
  EXPECT_EQ(result->positional[1], "pos2");
  EXPECT_EQ(result->named.at("--output")[0], "file.txt");
  EXPECT_TRUE(result->named.at("--verbose").empty());
  EXPECT_EQ(result->named.at("-n")[0], "42");
}

// ---- success: equals syntax ----

TEST(Tokenize, EqualsSyntaxExactOne) {
  SpecMap spec{{"--output", one}};
  auto result = tokenize({"--output=file.txt"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->positional.empty());
  EXPECT_EQ(result->named.at("--output")[0], "file.txt");
}

TEST(Tokenize, EqualsSyntaxValueContainsEquals) {
  // Value itself contains '=' — everything after the first '=' is the value
  SpecMap spec{{"--expr", one}};
  auto result = tokenize({"--expr=a=b"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->named.at("--expr")[0], "a=b");
}

TEST(Tokenize, EqualsSyntaxEmptyValue) {
  SpecMap spec{{"--tag", one}};
  auto result = tokenize({"--tag="}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->named.at("--tag")[0], "");
}

TEST(Tokenize, EqualsSyntaxNonExactOneGoesToPositional) {
  // --foo=bar should be treated as positional when nargs is not exact<1>
  SpecMap spec{{"--list", zero_or_more}};
  auto result = tokenize({"--list=item"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->named.empty());
  ASSERT_EQ(result->positional.size(), 1u);
  EXPECT_EQ(result->positional[0], "--list=item");
}

TEST(Tokenize, EqualsSyntaxFlagGoesToPositional) {
  SpecMap spec{{"--verbose", none}};
  auto result = tokenize({"--verbose=true"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->named.empty());
  EXPECT_EQ(result->positional[0], "--verbose=true");
}

TEST(Tokenize, EqualsSyntaxUnknownKeyGoesToPositional) {
  SpecMap spec{{"--foo", one}};
  auto result = tokenize({"--unknown=bar"}, spec);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->named.empty());
  EXPECT_EQ(result->positional[0], "--unknown=bar");
}

// ---- error: duplicate (all 4 path combinations) ----

// normal → normal: duplicate check at normal-path line
TEST(Tokenize, ErrorDuplicateNormalThenNormal) {
  SpecMap spec{{"--foo", one}};
  auto result = tokenize({"--foo", "a", "--foo", "b"}, spec);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--foo' specified multiple times");
}

// equals → equals: duplicate check at equals-path line
TEST(Tokenize, ErrorDuplicateEqualsThenEquals) {
  SpecMap spec{{"--foo", one}};
  auto result = tokenize({"--foo=a", "--foo=b"}, spec);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--foo' specified multiple times");
}

// normal → equals: first normal populates named, equals-path detects duplicate
TEST(Tokenize, ErrorDuplicateNormalThenEquals) {
  SpecMap spec{{"--foo", one}};
  auto result = tokenize({"--foo", "a", "--foo=b"}, spec);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--foo' specified multiple times");
}

// equals → normal: first equals populates named, normal-path detects duplicate
TEST(Tokenize, ErrorDuplicateEqualsThenNormal) {
  SpecMap spec{{"--foo", one}};
  auto result = tokenize({"--foo=a", "--foo", "b"}, spec);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--foo' specified multiple times");
}

TEST(Tokenize, ErrorDuplicateShortOption) {
  SpecMap spec{{"-f", one}};
  auto result = tokenize({"-f", "a", "-f", "b"}, spec);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '-f' specified multiple times");
}

TEST(Tokenize, ErrorDuplicateFlag) {
  SpecMap spec{{"--verbose", none}};
  auto result = tokenize({"--verbose", "--verbose"}, spec);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--verbose' specified multiple times");
}

TEST(Tokenize, ErrorDuplicateCommandName) {
  SpecMap spec{{"sub", {.min = 0, .max = std::nullopt}}};
  auto result = tokenize({"sub", "sub"}, spec);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option 'sub' specified multiple times");
}

// ---- error: below min ----

TEST(Tokenize, ErrorBelowMinAtEndOfArgs) {
  SpecMap spec{{"--foo", one}};
  auto result = tokenize({"--foo"}, spec);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--foo' requires at least 1 argument(s), but got 0");
}

TEST(Tokenize, ErrorBelowMinStopsAtNextOption) {
  SpecMap spec{{"--foo", one}, {"--bar", none}};
  auto result = tokenize({"--foo", "--bar"}, spec);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--foo' requires at least 1 argument(s), but got 0");
}

TEST(Tokenize, ErrorBelowMinExactTwoGotOne) {
  SpecMap spec{{"--range", exactly<2>}};
  auto result = tokenize({"--range", "1"}, spec);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--range' requires at least 2 argument(s), but got 1");
}

TEST(Tokenize, ErrorBelowMinExactTwoGotZero) {
  SpecMap spec{{"--range", exactly<2>}};
  auto result = tokenize({"--range"}, spec);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--range' requires at least 2 argument(s), but got 0");
}

TEST(Tokenize, ErrorBelowMinShortOption) {
  SpecMap spec{{"-o", one}};
  auto result = tokenize({"-o"}, spec);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '-o' requires at least 1 argument(s), but got 0");
}

TEST(Tokenize, ErrorBelowMinOneOrMore) {
  SpecMap spec{{"--list", one_or_more}};
  auto result = tokenize({"--list"}, spec);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "option '--list' requires at least 1 argument(s), but got 0");
}

