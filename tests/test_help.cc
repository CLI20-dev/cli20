#include <gtest/gtest.h>

#include "argon/arithmetic_argument.hh"
#include "argon/bool_argument.hh"
#include "argon/color.hh"
#include "argon/flag_argument.hh"
#include "argon/parser.hh"
#include "argon/string_argument.hh"

using namespace argon;

// ============================================================
// Testing philosophy
// ============================================================
//
// These tests verify that formatHelp() *contains* the required information,
// not that it reproduces a fixed format string.  Checking only for the
// presence of key substrings means:
//
//   - Changing spacing, indentation, or column width does not break tests.
//   - Adding new sections or styling (color, bold, underline) does not break
//     tests that are not specifically about styling.
//   - Tests remain readable and clearly communicate what they are asserting.
//
// Exceptions to this rule:
//   - Alignment tests deliberately check column positions — they must be
//     updated if the alignment logic changes intentionally.
//   - Color tests check for specific ANSI escape sequences because that is
//     the exact contract being verified.
//
// When adding new tests, ask: "Does this test break if only the visual
// layout changes?"  If yes, reconsider whether the layout is load-bearing.

// ============================================================
// Shared argument structs
// ============================================================

struct NoDescArgs {
  IntArg<"count", 'n'> count;
  FlagArg<"verbose", 'v'> verbose;
  IntPositional x;
};

struct WithDescArgs {
  IntArg<"count", 'n'> count{"Number of iterations"};
  FlagArg<"verbose", 'v'> verbose{"Enable verbose output"};
  StrArg<"output", 'o'> output{"Output file path"};
  IntPositional x{"Input value"};
};

struct RequiredDescArgs {
  StrArg<"remote"> remote{required, "Remote name"};
  IntArg<"depth", 'd'> depth{"Maximum depth"};
};

struct CommandDescArgs {
  FlagArg<"verbose", 'v'> verbose{"Show details"};
  struct PushSub {
    StrArg<"remote", 'r'> remote{"Remote repository"};
  };
  Command<PushSub, "push"> push{"Push changes to remote"};
};

struct MultiPositionalArgs {
  StrPositional src{"Source path"};
  StrPositional dst{"Destination path"};
};

// ============================================================
// Usage line
// ============================================================

TEST(FormatHelp, UsageProgramName) {
  // Program name after a successful parse must appear in the usage line.
  Parser<NoDescArgs> parser;
  std::vector<std::string_view> args = {"myprog"};
  parser.parse(args);
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("myprog"), std::string::npos);
  EXPECT_TRUE(help.starts_with("Usage:"));
}

TEST(FormatHelp, UsageBeforeParseFallsBackToDefault) {
  // Before any parse call the program name defaults to "program".
  Parser<NoDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("program"), std::string::npos);
}

TEST(FormatHelp, UsageContainsOptionsPlaceholder) {
  // "[options]" must appear when there are named options or flags.
  Parser<NoDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("[options]"), std::string::npos);
}

TEST(FormatHelp, UsageContainsPositionalMetavar) {
  // Positional metavars must appear in the usage line.
  Parser<NoDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("<int>"), std::string::npos);
}

TEST(FormatHelp, UsageContainsCommandPlaceholder) {
  // "[command]" must appear when at least one Command field is present.
  Parser<CommandDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("[command]"), std::string::npos);
}

TEST(FormatHelp, UsageNoOptionsPlaceholderWhenOnlyPositionals) {
  // "[options]" must NOT appear when there are no options or flags.
  Parser<MultiPositionalArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_EQ(help.find("[options]"), std::string::npos);
}

TEST(FormatHelp, UsageNoCommandPlaceholderWhenNoCommands) {
  // "[command]" must NOT appear when there are no Command fields.
  Parser<NoDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_EQ(help.find("[command]"), std::string::npos);
}

TEST(FormatHelp, UsageMultiplePositionals) {
  // All positional metavars appear in usage when multiple are declared.
  Parser<MultiPositionalArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  // Both <string> entries must be present (usage shows one per field).
  const auto first = help.find("<string>");
  ASSERT_NE(first, std::string::npos);
  EXPECT_NE(help.find("<string>", first + 1), std::string::npos);
}

// ============================================================
// Options section content
// ============================================================

TEST(FormatHelp, OptionLongNamesAppear) {
  // Long option names must appear in the Options section.
  Parser<NoDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("--count"), std::string::npos);
  EXPECT_NE(help.find("--verbose"), std::string::npos);
}

TEST(FormatHelp, OptionShortNamesAppear) {
  // Short option characters must appear alongside their long names.
  Parser<NoDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("-n"), std::string::npos);
  EXPECT_NE(help.find("-v"), std::string::npos);
}

TEST(FormatHelp, OptionWithoutShortOptHasFourSpacePadding) {
  // When there is no short option, the long name is indented with 4 spaces
  // to align with entries that have "-x, ".
  struct NoShortArgs {
    StrArg<"remote"> remote;  // no short opt
  };
  Parser<NoShortArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("    --remote"), std::string::npos);
}

TEST(FormatHelp, OptionMetavarIntAppearsForIntArg) {
  Parser<NoDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("<int>"), std::string::npos);
}

TEST(FormatHelp, OptionMetavarStringAppearsForStrArg) {
  struct StrArgStruct {
    StrArg<"file", 'f'> file;
  };
  Parser<StrArgStruct> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("<string>"), std::string::npos);
}

TEST(FormatHelp, OptionMetavarFloatAppearsForFloatArg) {
  struct FloatArgStruct {
    FloatArg<"ratio", 'r'> ratio;
  };
  Parser<FloatArgStruct> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("<float>"), std::string::npos);
}

TEST(FormatHelp, OptionMetavarDoubleAppearsForDoubleArg) {
  struct DoubleArgStruct {
    DoubleArg<"pi"> pi;
  };
  Parser<DoubleArgStruct> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("<double>"), std::string::npos);
}

TEST(FormatHelp, OptionMetavarUintAppearsForUintArg) {
  struct UintArgStruct {
    Uint32Arg<"size", 's'> size;
  };
  Parser<UintArgStruct> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("<uint>"), std::string::npos);
}

TEST(FormatHelp, OptionMetavarBoolAppearsForBoolArg) {
  struct BoolArgStruct {
    BoolArg<"flag", 'f'> flag;
  };
  Parser<BoolArgStruct> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("<bool>"), std::string::npos);
}

TEST(FormatHelp, OptionMetavarListAppearsForListArg) {
  // Vector types show "<element-type...>" as the metavar.
  struct ListArgStruct {
    IntListArg<"nums", 'n'> nums;
  };
  Parser<ListArgStruct> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("<int...>"), std::string::npos);
}

TEST(FormatHelp, FlagHasNoMetavar) {
  // A flag (nargs=0) must NOT show a <type> metavar on its help line.
  Parser<WithDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  const auto pos = help.find("--verbose");
  ASSERT_NE(pos, std::string::npos);
  const auto eol = help.find('\n', pos);
  const auto line = help.substr(pos, eol - pos);
  EXPECT_EQ(line.find('<'), std::string::npos);
}

TEST(FormatHelp, OptionsSectionHeaderPresent) {
  // "Options:" header must appear whenever there are options or flags.
  Parser<NoDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("Options:"), std::string::npos);
}

TEST(FormatHelp, NoOptionsSectionWhenNoOptionsOrFlags) {
  // "Options:" must NOT appear when the struct has only positionals.
  Parser<MultiPositionalArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_EQ(help.find("Options:"), std::string::npos);
}

// ============================================================
// Descriptions
// ============================================================

TEST(FormatHelp, DescriptionsAppearWhenSet) {
  // Every description string must appear verbatim somewhere in the output.
  Parser<WithDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("Number of iterations"), std::string::npos);
  EXPECT_NE(help.find("Enable verbose output"), std::string::npos);
  EXPECT_NE(help.find("Output file path"), std::string::npos);
  EXPECT_NE(help.find("Input value"), std::string::npos);
}

TEST(FormatHelp, NoGarbageWhenDescriptionsUnset) {
  // Output must still be valid (non-empty, starts with "Usage:") when no
  // argument carries a description.
  Parser<NoDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_FALSE(help.empty());
  EXPECT_TRUE(help.starts_with("Usage:"));
}

TEST(FormatHelp, CommandDescriptionAppears) {
  Parser<CommandDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("Push changes to remote"), std::string::npos);
}

// ============================================================
// Alignment — descriptions at a consistent column
// ============================================================

TEST(FormatHelp, DescriptionsAreAligned) {
  // All descriptions within the same section must start at the same column.
  // This test is intentionally sensitive to alignment logic; update it when
  // the alignment algorithm changes deliberately.
  struct AlignArgs {
    IntArg<"short", 's'> s{"Short desc"};
    IntArg<"very-long-option-name"> vlong{"Long desc"};
  };
  Parser<AlignArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);

  // Find the column (offset from line start) of each description.
  auto desc_column = [&](std::string_view desc) -> std::size_t {
    const auto pos = help.find(desc);
    if (pos == std::string::npos) return std::string::npos;
    const auto line_start = help.rfind('\n', pos);
    return pos - (line_start == std::string::npos ? 0 : line_start + 1);
  };

  const auto col_short = desc_column("Short desc");
  const auto col_long = desc_column("Long desc");
  ASSERT_NE(col_short, std::string::npos);
  ASSERT_NE(col_long, std::string::npos);
  EXPECT_EQ(col_short, col_long);
}

// ============================================================
// Positional arguments section
// ============================================================

TEST(FormatHelp, PositionalSectionHeaderPresent) {
  Parser<WithDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("Positional arguments:"), std::string::npos);
}

TEST(FormatHelp, MultiplePositionalsDescriptionsAppear) {
  // Each positional's description must be present; they cannot be
  // distinguished by metavar alone when two have the same type.
  Parser<MultiPositionalArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("Source path"), std::string::npos);
  EXPECT_NE(help.find("Destination path"), std::string::npos);
}

TEST(FormatHelp, NoPositionalSectionWhenNoPositionals) {
  struct OnlyOptions {
    IntArg<"count", 'n'> count;
  };
  Parser<OnlyOptions> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_EQ(help.find("Positional arguments:"), std::string::npos);
}

// ============================================================
// Commands section
// ============================================================

TEST(FormatHelp, CommandsSectionHeaderPresent) {
  Parser<CommandDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("Commands:"), std::string::npos);
}

TEST(FormatHelp, CommandNameAppears) {
  Parser<CommandDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_NE(help.find("push"), std::string::npos);
}

TEST(FormatHelp, NoCommandsSectionWhenNoCommands) {
  Parser<NoDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_EQ(help.find("Commands:"), std::string::npos);
}

// ============================================================
// Section ordering
// ============================================================

TEST(FormatHelp, OptionsSectionBeforePositionalsSectionBeforeCommands) {
  // The canonical order is Options → Positional arguments → Commands.
  struct AllThree {
    IntArg<"count", 'n'> count;
    IntPositional x;
    struct Sub {};
    Command<Sub, "run"> run;
  };
  Parser<AllThree> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  const auto opt_pos = help.find("Options:");
  const auto pos_pos = help.find("Positional arguments:");
  const auto cmd_pos = help.find("Commands:");
  ASSERT_NE(opt_pos, std::string::npos);
  ASSERT_NE(pos_pos, std::string::npos);
  ASSERT_NE(cmd_pos, std::string::npos);
  EXPECT_LT(opt_pos, pos_pos);
  EXPECT_LT(pos_pos, cmd_pos);
}

// ============================================================
// Color mode: ColorMode::never
// ============================================================

TEST(FormatHelp, ColorNeverProducesNoAnsiEscapeCodes) {
  // ColorMode::never must never emit any ESC sequence regardless of terminal.
  Parser<WithDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::never);
  EXPECT_EQ(help.find("\033["), std::string::npos);
}

TEST(FormatHelp, ColorNeverContentMatchesColorAlwaysContent) {
  // Plain-text content (descriptions, option names, etc.) must be identical
  // regardless of color mode; only the ANSI codes differ.
  Parser<WithDescArgs> parser;
  const auto plain = parser.formatHelp(ColorMode::never);
  const auto color = parser.formatHelp(ColorMode::always);

  // Strip all ANSI sequences from the colored version and compare.
  std::string stripped;
  stripped.reserve(color.size());
  for (std::size_t i = 0; i < color.size(); ) {
    if (color[i] == '\033' && i + 1 < color.size() && color[i + 1] == '[') {
      // Skip until 'm'
      i += 2;
      while (i < color.size() && color[i] != 'm') ++i;
      ++i;  // skip 'm'
    } else {
      stripped += color[i++];
    }
  }
  EXPECT_EQ(plain, stripped);
}

// ============================================================
// Color mode: ColorMode::always
// ============================================================

TEST(FormatHelp, ColorAlwaysEmitsBoldCode) {
  // ColorMode::always must include the SGR bold sequence (\033[1m).
  Parser<WithDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::always);
  EXPECT_NE(help.find("\033[1m"), std::string::npos);
}

TEST(FormatHelp, ColorAlwaysEmitsUnderlineCode) {
  // ColorMode::always must include the SGR underline sequence (\033[4m).
  Parser<WithDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::always);
  EXPECT_NE(help.find("\033[4m"), std::string::npos);
}

TEST(FormatHelp, ColorAlwaysEmitsResetCode) {
  // Every SGR activation must be followed by a reset (\033[0m).
  Parser<WithDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::always);
  EXPECT_NE(help.find("\033[0m"), std::string::npos);
}

TEST(FormatHelp, ColorAlwaysSectionHeaderIsBoldAndUnderlined) {
  // Section headers must be bold+underlined: \033[1m\033[4m<header>\033[0m
  Parser<WithDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::always);
  // Look for bold+underline prefix immediately before a known header word.
  EXPECT_NE(help.find("\033[1m\033[4mOptions"), std::string::npos);
}

TEST(FormatHelp, ColorAlwaysOptionNameIsBold) {
  // Option left column entries must be wrapped in bold.
  Parser<WithDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::always);
  // Bold must appear immediately before the known option text.
  EXPECT_NE(help.find("\033[1m-n, --count"), std::string::npos);
}

TEST(FormatHelp, NoRgbEscapeSequences) {
  // The library must not emit RGB/truecolor sequences (38;2 or 48;2).
  // Standard 8/16-color SGR only.
  Parser<WithDescArgs> parser;
  const auto help = parser.formatHelp(ColorMode::always);
  EXPECT_EQ(help.find("38;2"), std::string::npos);
  EXPECT_EQ(help.find("48;2"), std::string::npos);
}

// ============================================================
// AnsiStyle unit tests
// ============================================================

TEST(AnsiStyle, DisabledReturnsEmptyStrings) {
  const detail::AnsiStyle off{false};
  EXPECT_TRUE(off.bold().empty());
  EXPECT_TRUE(off.underline().empty());
  EXPECT_TRUE(off.reset().empty());
}

TEST(AnsiStyle, EnabledReturnsCodes) {
  const detail::AnsiStyle on{true};
  EXPECT_EQ(on.bold(), "\033[1m");
  EXPECT_EQ(on.underline(), "\033[4m");
  EXPECT_EQ(on.reset(), "\033[0m");
}

TEST(AnsiStyle, ResolveColorNeverIsDisabled) {
  const auto style = detail::resolveColor(ColorMode::never);
  EXPECT_FALSE(style.enabled);
}

TEST(AnsiStyle, ResolveColorAlwaysIsEnabled) {
  const auto style = detail::resolveColor(ColorMode::always);
  EXPECT_TRUE(style.enabled);
}

// ============================================================
// description() accessor — constructor variants
// ============================================================

TEST(DescriptionConstructor, DescOnlyConstructorSetsDescriptionAndKeepsOptional) {
  IntArg<"count", 'n'> arg{"My description"};
  EXPECT_EQ(arg.description(), "My description");
  EXPECT_FALSE(arg.isRequired());
}

TEST(DescriptionConstructor, RequiredPlusDescriptionConstructor) {
  StrArg<"remote"> arg{required, "Remote name"};
  EXPECT_EQ(arg.description(), "Remote name");
  EXPECT_TRUE(arg.isRequired());
}

TEST(DescriptionConstructor, FlagDescriptionConstructor) {
  FlagArg<"verbose", 'v'> flag{"Toggle verbose"};
  EXPECT_EQ(flag.description(), "Toggle verbose");
}

TEST(DescriptionConstructor, IntPositionalDescriptionConstructor) {
  IntPositional pos{"The input integer"};
  EXPECT_EQ(pos.description(), "The input integer");
  EXPECT_FALSE(pos.isRequired());
}

TEST(DescriptionConstructor, StrPositionalDescriptionConstructor) {
  StrPositional pos{"A file path"};
  EXPECT_EQ(pos.description(), "A file path");
}

TEST(DescriptionConstructor, CommandDescriptionConstructor) {
  struct Sub {};
  Command<Sub, "push"> cmd{"Push changes"};
  EXPECT_EQ(cmd.description(), "Push changes");
}

TEST(DescriptionConstructor, DefaultDescriptionIsEmpty) {
  IntArg<"count", 'n'> arg;
  EXPECT_TRUE(arg.description().empty());
  FlagArg<"verbose", 'v'> flag;
  EXPECT_TRUE(flag.description().empty());
  IntPositional pos;
  EXPECT_TRUE(pos.description().empty());
}
