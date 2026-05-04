#include <gtest/gtest.h>

#include "argon/arithmetic_argument.hh"
#include "argon/flag_argument.hh"
#include "argon/parser.hh"
#include "argon/string_argument.hh"

// ---- Helpers ----------------------------------------------------------------

static auto sv(std::initializer_list<const char*> strs) -> std::vector<std::string_view> {
  return {strs.begin(), strs.end()};
}

// ---- Structs ----------------------------------------------------------------

struct SimpleArgs {
  argon::HelpFlag<> help;
  argon::StrArg<"output", 'o'> output{"Output file"};
};

struct RequiredArgs {
  argon::HelpFlag<> help;
  argon::StrArg<"remote", 'r'> remote{argon::required, "Remote name"};
  argon::IntArg<"count", 'n'> count{argon::required, "Count"};
};

struct CustomHelpArgs {
  argon::HelpFlag<"version", 'V'> version{"Show version"};
  argon::StrArg<"output", 'o'> output;
};

struct NoShortHelpArgs {
  argon::HelpFlag<"info"> info;
  argon::StrArg<"output", 'o'> output;
};

struct SubArgs {
  argon::StrArg<"target", 't'> target{"Build target"};
  argon::StrArg<"extra", 'e'> extra{argon::required, "Extra (required)"};
};

struct TopArgs {
  argon::HelpFlag<> help;
  argon::StrArg<"config", 'c'> config{"Config file"};
  argon::Command<SubArgs, "build"> build{"Compile the project"};
};

struct PositionalArgs {
  argon::HelpFlag<> help;
  argon::StrPositional src{argon::required, "Source"};
};

// ---- Default template params ------------------------------------------------

TEST(HelpFlag, DefaultLongOpt) {
  argon::Parser<SimpleArgs> parser;
  auto res = parser.parse(sv({"prog", "--help"}));
  ASSERT_TRUE(res.has_value());
  EXPECT_TRUE(res->help.provided());
}

TEST(HelpFlag, DefaultShortOpt) {
  argon::Parser<SimpleArgs> parser;
  auto res = parser.parse(sv({"prog", "-h"}));
  ASSERT_TRUE(res.has_value());
  EXPECT_TRUE(res->help.provided());
}

TEST(HelpFlag, NotProvidedWhenAbsent) {
  argon::Parser<SimpleArgs> parser;
  auto res = parser.parse(sv({"prog"}));
  ASSERT_TRUE(res.has_value());
  EXPECT_FALSE(res->help.provided());
}

// ---- Custom template params -------------------------------------------------

TEST(HelpFlag, CustomLongOpt) {
  argon::Parser<CustomHelpArgs> parser;
  auto res = parser.parse(sv({"prog", "--version"}));
  ASSERT_TRUE(res.has_value());
  EXPECT_TRUE(res->version.provided());
}

TEST(HelpFlag, CustomShortOpt) {
  argon::Parser<CustomHelpArgs> parser;
  auto res = parser.parse(sv({"prog", "-V"}));
  ASSERT_TRUE(res.has_value());
  EXPECT_TRUE(res->version.provided());
}

TEST(HelpFlag, NoShortOpt) {
  argon::Parser<NoShortHelpArgs> parser;
  auto res = parser.parse(sv({"prog", "--info"}));
  ASSERT_TRUE(res.has_value());
  EXPECT_TRUE(res->info.provided());
}

// ---- Error bypass -----------------------------------------------------------

TEST(HelpFlag, BypassesMissingRequired) {
  // Without --help, missing --remote and --count would be errors.
  argon::Parser<RequiredArgs> parser;
  auto no_help = parser.parse(sv({"prog"}));
  EXPECT_FALSE(no_help.has_value());

  // With --help, parse succeeds even though required args are absent.
  auto with_help = parser.parse(sv({"prog", "--help"}));
  ASSERT_TRUE(with_help.has_value());
  EXPECT_TRUE(with_help->help.provided());
}

TEST(HelpFlag, BypassesUnknownOption) {
  argon::Parser<SimpleArgs> parser;
  // --help comes before an unknown option — should succeed.
  auto res = parser.parse(sv({"prog", "--help", "--unknown"}));
  ASSERT_TRUE(res.has_value());
  EXPECT_TRUE(res->help.provided());
}

TEST(HelpFlag, BypassesMissingRequiredPositional) {
  argon::Parser<PositionalArgs> parser;
  auto no_help = parser.parse(sv({"prog"}));
  EXPECT_FALSE(no_help.has_value());

  auto with_help = parser.parse(sv({"prog", "--help"}));
  ASSERT_TRUE(with_help.has_value());
  EXPECT_TRUE(with_help->help.provided());
}

// ---- Order independence -----------------------------------------------------

TEST(HelpFlag, HelpAfterOtherOption) {
  argon::Parser<SimpleArgs> parser;
  auto res = parser.parse(sv({"prog", "--output", "file.txt", "--help"}));
  ASSERT_TRUE(res.has_value());
  EXPECT_TRUE(res->help.provided());
}

TEST(HelpFlag, HelpBeforeOtherOption) {
  argon::Parser<SimpleArgs> parser;
  auto res = parser.parse(sv({"prog", "--help", "--output", "file.txt"}));
  ASSERT_TRUE(res.has_value());
  EXPECT_TRUE(res->help.provided());
}

// ---- End-of-options separator stops scan -----------------------------------

TEST(HelpFlag, AfterSeparatorDoesNotTrigger) {
  // "-- --help" should NOT trigger HelpFlag; "--help" is treated as a positional.
  // SimpleArgs has no positionals, so parse should fail with unexpected_argument.
  argon::Parser<SimpleArgs> parser;
  auto res = parser.parse(sv({"prog", "--", "--help"}));
  EXPECT_FALSE(res.has_value());
}

// ---- Command name stops scan -----------------------------------------------

TEST(HelpFlag, AfterCommandNameDoesNotTrigger) {
  // "build --help" — help is inside the sub-command scope, not top-level.
  // The top-level help should NOT be marked provided.
  argon::Parser<TopArgs> parser;
  auto res = parser.parse(sv({"prog", "build", "--help"}));
  // Sub-parser for SubArgs has no HelpFlag, so --help is an unknown option.
  EXPECT_FALSE(res.has_value());
}

TEST(HelpFlag, TopLevelHelpBeforeCommand) {
  argon::Parser<TopArgs> parser;
  auto res = parser.parse(sv({"prog", "--help", "build"}));
  ASSERT_TRUE(res.has_value());
  EXPECT_TRUE(res->help.provided());
}

// ---- Recursive formatHelp ---------------------------------------------------

struct RecurseSubArgs {
  argon::StrArg<"target", 't'> target{"Build target"};
  argon::FlagArg<"dry-run"> dry_run{"Dry run mode"};
};

struct RecurseTopArgs {
  argon::HelpFlag<> help;
  argon::FlagArg<"verbose", 'v'> verbose{"Verbose output"};
  argon::Command<RecurseSubArgs, "build"> build{"Compile the project"};
};

TEST(FormatHelpRecurse, ContainsSubCommandSection) {
  argon::Parser<RecurseTopArgs> parser;
  const auto h = parser.formatHelp(argon::ColorMode::never, argon::recurseHelp);
  EXPECT_NE(h.find("build"), std::string::npos);
  EXPECT_NE(h.find("target"), std::string::npos);
  EXPECT_NE(h.find("dry-run"), std::string::npos);
}

TEST(FormatHelpRecurse, SubCommandProgramNameIncludesParent) {
  argon::Parser<RecurseTopArgs> parser;
  const auto h = parser.formatHelp(argon::ColorMode::never, argon::recurseHelp);
  // The sub-command usage line should say "prog build"
  EXPECT_NE(h.find("prog build"), std::string::npos);
}

TEST(FormatHelpRecurse, NonRecurseDoesNotContainSubArgs) {
  argon::Parser<RecurseTopArgs> parser;
  const auto h = parser.formatHelp(argon::ColorMode::never);
  // Without recurse, --target from sub-command should NOT appear.
  EXPECT_EQ(h.find("--target"), std::string::npos);
  EXPECT_EQ(h.find("--dry-run"), std::string::npos);
}

TEST(FormatHelpRecurse, ContainsCommandSectionHeader) {
  argon::Parser<RecurseTopArgs> parser;
  const auto h = parser.formatHelp(argon::ColorMode::never, argon::recurseHelp);
  // Should contain a separator line with the command name (─── build ───)
  EXPECT_NE(h.find(" build "), std::string::npos);
}
