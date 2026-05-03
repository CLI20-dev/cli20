#include <gtest/gtest.h>

#include "argon/arithmetic_argument.hh"
#include "argon/arithmetic_positional.hh"
#include "argon/bool_argument.hh"
#include "argon/parser.hh"
#include "argon/string_argument.hh"

using namespace argon;

// ---- Argument structs ----

struct PushSubArgs {
  StrArg<"remote", 'r'> remote;
  FlagArg<"force", 'f'> force;
  IntArg<"depth", 'd'> depth;
};

struct TopLevelArgs {
  FlagArg<"verbose", 'v'> verbose;
  IntArg<"jobs", 'j'> jobs;
  Command<PushSubArgs, "push"> push;
};

struct TwoCommandArgs {
  struct AddSub {
    StrPositional file;
  };
  struct RmSub {
    FlagArg<"recursive", 'r'> recursive;
    StrPositional file;
  };
  Command<AddSub, "add"> add;
  Command<RmSub, "rm"> rm;
};

struct MixedPositionalCommandArgs {
  StrPositional repo;
  Command<PushSubArgs, "push"> push;
};

// ============================================================
// Basic command parsing
// ============================================================

TEST(ParseCommand, BasicCommand) {
  Parser<TopLevelArgs> parser;
  std::vector<std::string_view> args = {"prog", "push", "--remote", "origin"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->push.provided());
  EXPECT_EQ(result->push.remote.value(), "origin");
  EXPECT_FALSE(result->push.force.provided());
}

TEST(ParseCommand, CommandWithFlag) {
  Parser<TopLevelArgs> parser;
  std::vector<std::string_view> args = {"prog", "push", "--force"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->push.provided());
  EXPECT_TRUE(result->push.force.provided());
  EXPECT_FALSE(result->push.remote.provided());
}

TEST(ParseCommand, CommandWithMultipleOptions) {
  Parser<TopLevelArgs> parser;
  std::vector<std::string_view> args = {"prog",    "push",    "--remote", "upstream",
                                        "--force", "--depth", "3"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->push.provided());
  EXPECT_EQ(result->push.remote.value(), "upstream");
  EXPECT_TRUE(result->push.force.provided());
  EXPECT_EQ(result->push.depth.value(), 3);
}

TEST(ParseCommand, CommandShortOptions) {
  Parser<TopLevelArgs> parser;
  std::vector<std::string_view> args = {"prog", "push", "-r", "origin", "-f", "-d", "5"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->push.provided());
  EXPECT_EQ(result->push.remote.value(), "origin");
  EXPECT_TRUE(result->push.force.provided());
  EXPECT_EQ(result->push.depth.value(), 5);
}

// ============================================================
// Top-level options before command
// ============================================================

TEST(ParseCommand, TopLevelFlagThenCommand) {
  Parser<TopLevelArgs> parser;
  std::vector<std::string_view> args = {"prog", "--verbose", "push", "--remote", "origin"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->verbose.provided());
  EXPECT_TRUE(result->push.provided());
  EXPECT_EQ(result->push.remote.value(), "origin");
}

TEST(ParseCommand, TopLevelOptionAndFlagThenCommand) {
  Parser<TopLevelArgs> parser;
  std::vector<std::string_view> args = {"prog", "-v", "--jobs", "4", "push", "--force"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->verbose.provided());
  EXPECT_EQ(result->jobs.value(), 4);
  EXPECT_TRUE(result->push.provided());
  EXPECT_TRUE(result->push.force.provided());
}

// ============================================================
// Command not provided
// ============================================================

TEST(ParseCommand, NoCommand) {
  Parser<TopLevelArgs> parser;
  std::vector<std::string_view> args = {"prog", "--verbose"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->verbose.provided());
  EXPECT_FALSE(result->push.provided());
}

TEST(ParseCommand, EmptyArgs) {
  Parser<TopLevelArgs> parser;
  std::vector<std::string_view> args = {"prog"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->verbose.provided());
  EXPECT_FALSE(result->push.provided());
}

// ============================================================
// Two sibling commands
// ============================================================

TEST(ParseCommand, FirstOfTwoCommands) {
  Parser<TwoCommandArgs> parser;
  std::vector<std::string_view> args = {"prog", "add", "main.cc"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->add.provided());
  EXPECT_FALSE(result->rm.provided());
  EXPECT_EQ(result->add.file.value(), "main.cc");
}

TEST(ParseCommand, SecondOfTwoCommands) {
  Parser<TwoCommandArgs> parser;
  std::vector<std::string_view> args = {"prog", "rm", "-r", "src"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->add.provided());
  EXPECT_TRUE(result->rm.provided());
  EXPECT_TRUE(result->rm.recursive.provided());
  EXPECT_EQ(result->rm.file.value(), "src");
}

// ============================================================
// Top-level positional before command
// ============================================================

TEST(ParseCommand, PositionalThenCommand) {
  Parser<MixedPositionalCommandArgs> parser;
  std::vector<std::string_view> args = {"prog", "my-repo", "push", "--remote", "origin"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->repo.value(), "my-repo");
  EXPECT_TRUE(result->push.provided());
  EXPECT_EQ(result->push.remote.value(), "origin");
}

TEST(ParseCommand, PositionalNoCommand) {
  Parser<MixedPositionalCommandArgs> parser;
  std::vector<std::string_view> args = {"prog", "my-repo"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->repo.value(), "my-repo");
  EXPECT_FALSE(result->push.provided());
}

// ============================================================
// Error propagation from sub-command
// ============================================================

TEST(ParseCommand, SubCommandRequiredOptionMissing) {
  struct StrictSub {
    StrArg<"remote"> remote{required};
  };
  struct StrictTop {
    Command<StrictSub, "push"> push;
  };
  Parser<StrictTop> parser;
  std::vector<std::string_view> args = {"prog", "push"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "required option '--remote' was not provided");
}

TEST(ParseCommand, SubCommandInvalidOption) {
  Parser<TopLevelArgs> parser;
  std::vector<std::string_view> args = {"prog", "push", "--depth", "abc"};
  auto result = parser.parse(args);
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

// ============================================================
// "--" end-of-options separator
// ============================================================

struct StrPositionalArgs {
  StrPositional a;
  StrPositional b;
  StrPositional c;
};

struct OptionAndStrPositionalArgs {
  StrArg<"output", 'o'> output;
  StrPositional file;
};

TEST(ParseDoubleDash, AllAfterSeparatorArePositional) {
  Parser<StrPositionalArgs> parser;
  // "--foo" and "-x" look like option names but come after "--"
  std::vector<std::string_view> args = {"prog", "--", "--foo", "-x", "bar"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->a.value(), "--foo");
  EXPECT_EQ(result->b.value(), "-x");
  EXPECT_EQ(result->c.value(), "bar");
}

TEST(ParseDoubleDash, SeparatorAloneIsEmpty) {
  Parser<StrPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "--"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->a.provided());
  EXPECT_FALSE(result->b.provided());
}

TEST(ParseDoubleDash, OptionBeforeSeparatorThenPositional) {
  Parser<OptionAndStrPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "--output", "out.txt", "--", "--input.txt"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->output.value(), "out.txt");
  EXPECT_EQ(result->file.value(), "--input.txt");
}

TEST(ParseDoubleDash, PositionalBeforeSeparator) {
  Parser<StrPositionalArgs> parser;
  std::vector<std::string_view> args = {"prog", "first", "--", "second", "third"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->a.value(), "first");
  EXPECT_EQ(result->b.value(), "second");
  EXPECT_EQ(result->c.value(), "third");
}

TEST(ParseDoubleDash, CommandNameAfterSeparatorIsPositional) {
  // "push" after "--" is treated as a positional string, not a command
  Parser<MixedPositionalCommandArgs> parser;
  std::vector<std::string_view> args = {"prog", "--", "push"};
  auto result = parser.parse(args);
  ASSERT_TRUE(result.has_value());
  // "push" became a positional for `repo`
  EXPECT_EQ(result->repo.value(), "push");
  EXPECT_FALSE(result->push.provided());
}
