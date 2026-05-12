#include <gtest/gtest.h>

#include <string_view>
#include <vector>

#include "cli/argument.hh"
#include "cli/color.hh"
#include "cli/parser.hh"

namespace {

struct BuildArgs {
  cli::Description description{"Compile sources into an executable."};

  cli::Help<> help;
  cli::Flag<"release", 'r'> release{
      {.help = "Build with optimizations", .presence = cli::optional}};
  cli::IntOption<"jobs", 'j'> jobs{
      {.help = "Parallel job count", .presence = cli::required}};
};

struct HelpArgs {
  cli::Description description{
      "A small tool used to exercise the help generator."};

  cli::Flag<"verbose", 'v'> verbose{
      {.help = "Enable verbose logging", .presence = cli::optional}};
  cli::IntOption<"count", 'c'> count{
      {.help = "Number of iterations", .presence = cli::optional}};
  cli::ListOption<std::string, "include", 'I', cli::nargs::exactly<2>> includes{
      {.help = "Two include directories", .presence = cli::optional}};
  cli::Positional<std::string> input{
      {.help = "Input file", .presence = cli::required}};
  cli::Command<"build", BuildArgs> build{{.help = "Run the build step"}};
};

struct OptionsOnlyArgs {
  cli::Flag<"verbose", 'v'> verbose{
      {.help = "Enable verbose logging", .presence = cli::optional}};
};

struct MultilineHelpArgs {
  cli::StringOption<"mode"> mode{{
      .help = "First line\nSecond line",
      .default_value = std::string{"auto"},
  }};
};

struct CommandsOnlyArgs {
  cli::Command<"build", BuildArgs> build{{.help = "Run the build step"}};
};

struct HelpFlagArgs {
  cli::Help<> help;
};

struct CustomHelpFlagArgs {
  cli::Help<"usage", 'u'> usage;
};

struct ExitSuccessArgs {
  cli::Arg<"version", 'V', cli::action::exit_success, cli::nargs::none> version{
      {.help = "Print version and exit", .presence = cli::optional}};
};

struct MetadataHelpArgs {
  cli::Flag<"visible"> visible{{.help = "Visible flag"}};
  cli::Flag<"hidden"> hidden{{.help = "Hidden flag", .hidden = true}};
  cli::Flag<"old"> old{{
      .help = "Old flag",
      .deprecated = "use --visible instead",
  }};
  cli::StringOption<"output", 'o'> output{{
      .help = "Output file",
      .presence = cli::required,
      .default_value = std::string{"out.txt"},
      .env = "APP_OUTPUT",
  }};
  cli::StringOption<"user"> user{{.help = "User name"}};
  cli::StringOption<"password"> password{{.help = "Password"}};
  cli::Flag<"json"> json{{.help = "JSON output"}};
  cli::Flag<"markdown"> markdown{{.help = "Markdown output"}};
  cli::Flag<"deploy"> deploy{{.help = "Deploy"}};
  cli::StringOption<"profile"> profile{{.help = "Deploy profile"}};
  cli::Command<"legacy", BuildArgs> legacy{{
      .help = "Legacy command",
      .deprecated = "use build",
  }};
  cli::Command<"internal", BuildArgs> internal{{
      .help = "Internal command",
      .hidden = true,
  }};

  static constexpr auto relations = cli::relations(
      cli::group({.name = "legacy_auth", .members = {"user", "password"}}),
      cli::conflicts("json", "markdown"), cli::depends_on("deploy", "profile"));
};

}  // namespace

TEST(Help, UsageContainsProgramAndMajorGroups) {
  char program[] = "myprog";
  char* argv[] = {program};

  cli::Parser<HelpArgs> parser;

  std::ignore = parser.parse(1, argv);

  const auto help = parser.format_help(cli::ColorMode::never);
  EXPECT_TRUE(help.starts_with("Usage: myprog"));
  EXPECT_NE(help.find("[options]"), std::string::npos);
  EXPECT_NE(help.find("[args]"), std::string::npos);
  EXPECT_NE(help.find("[command]"), std::string::npos);
}

TEST(Help, DescriptionAndSectionsAppear) {
  cli::Parser<HelpArgs> parser;

  const auto help = parser.format_help(cli::ColorMode::never);
  EXPECT_NE(help.find("A small tool used to exercise the help generator."),
            std::string::npos);
  EXPECT_NE(help.find("Options:"), std::string::npos);
  EXPECT_NE(help.find("Positional arguments:"), std::string::npos);
  EXPECT_NE(help.find("Commands:"), std::string::npos);
}

TEST(Help, OptionAndPositionalMetavarsAreRendered) {
  cli::Parser<HelpArgs> parser;

  const auto help = parser.format_help(cli::ColorMode::never);
  EXPECT_NE(help.find("-c, --count <int>"), std::string::npos);
  EXPECT_NE(help.find("-I, --include <string...>"), std::string::npos);
  EXPECT_NE(help.find("<string>"), std::string::npos);
}

TEST(Help, FlagDoesNotShowMetavar) {
  cli::Parser<HelpArgs> parser;

  const auto help = parser.format_help(cli::ColorMode::never);
  const auto line_start = help.find("-v, --verbose");
  ASSERT_NE(line_start, std::string::npos);
  const auto line_end = help.find('\n', line_start);
  const auto line = help.substr(line_start, line_end - line_start);
  EXPECT_EQ(line.find("<bool>"), std::string::npos);
}

TEST(Help, MultilineHelpTextKeepsContinuationIndented) {
  cli::Parser<MultilineHelpArgs> parser;

  const auto help = parser.format_help(cli::ColorMode::never);
  EXPECT_NE(help.find("--mode <string>  First line [default: auto]\n"
                      "                   Second line\n"),
            std::string::npos);
}

TEST(Help, EmptySectionsAreOmitted) {
  cli::Parser<OptionsOnlyArgs> options_only;
  const auto options_help = options_only.format_help(cli::ColorMode::never);
  EXPECT_NE(options_help.find("Options:"), std::string::npos);
  EXPECT_EQ(options_help.find("Positional arguments:"), std::string::npos);
  EXPECT_EQ(options_help.find("Commands:"), std::string::npos);

  cli::Parser<CommandsOnlyArgs> commands_only;
  const auto commands_help = commands_only.format_help(cli::ColorMode::never);
  EXPECT_EQ(commands_help.find("Options:"), std::string::npos);
  EXPECT_EQ(commands_help.find("Positional arguments:"), std::string::npos);
  EXPECT_NE(commands_help.find("Commands:"), std::string::npos);
}

TEST(Help, RecursiveHelpIncludesSubcommandBody) {
  cli::Parser<HelpArgs> parser;

  const auto help = parser.format_help(cli::ColorMode::never, cli::recurse_help);
  EXPECT_NE(help.find("Usage: program build [options]"), std::string::npos);
  EXPECT_NE(help.find("Compile sources into an executable."), std::string::npos);
  EXPECT_NE(help.find("-j, --jobs <int>"), std::string::npos);
}

TEST(Help, ColorNeverDoesNotEmitAnsi) {
  cli::Parser<HelpArgs> parser;

  const auto help = parser.format_help(cli::ColorMode::never);
  EXPECT_EQ(help.find("\033["), std::string::npos);
}

TEST(Help, ColorAlwaysEmitsAnsiHeadingsAndLabels) {
  cli::Parser<HelpArgs> parser;

  const auto help = parser.format_help(cli::ColorMode::always);
  EXPECT_NE(help.find("\033[1m"), std::string::npos);
  EXPECT_NE(help.find("\033[1;4;36mOptions:"), std::string::npos);
  EXPECT_NE(help.find("\033[0m"), std::string::npos);
  EXPECT_NE(help.find("\033[1;32m-v\033[0m, \033[1;32m--verbose"),
            std::string::npos);
  EXPECT_NE(help.find("\033[36m<string>\033[0m"), std::string::npos);
}

TEST(Help, CustomPaletteIsUsed) {
  cli::Parser<HelpArgs> parser;
  constexpr cli::HelpPalette palette{
      .usage = "\033[95m",
      .heading = "\033[94m",
      .option = "\033[93m",
      .metavar = "\033[92m",
      .command = "\033[91m",
      .metadata = "\033[90m",
      .group = "\033[96m",
      .reset = "\033[39m",
  };

  const auto help = parser.format_help(cli::ColorMode::always, palette);
  EXPECT_NE(help.find("\033[95mUsage:\033[39m"), std::string::npos);
  EXPECT_NE(help.find("\033[94mOptions:\033[39m"), std::string::npos);
  EXPECT_NE(help.find("\033[93m-v\033[39m, \033[93m--verbose"),
            std::string::npos);
}

TEST(Help, ResolveColorRespectsMode) {
  const auto never = cli::detail::resolve_color(cli::ColorMode::never);
  const auto always = cli::detail::resolve_color(cli::ColorMode::always);

  EXPECT_EQ(never.bold(), "");
  EXPECT_EQ(never.underline(), "");
  EXPECT_EQ(never.reset(), "");

  EXPECT_EQ(always.bold(), "\033[1m");
  EXPECT_EQ(always.underline(), "\033[4m");
  EXPECT_EQ(always.reset(), "\033[0m");
  EXPECT_EQ(always.heading(), "\033[1;4;36m");
  EXPECT_EQ(always.option(), "\033[1;32m");
}

TEST(Help, HelpFlagTriggersHelpRequested) {
  auto args = std::vector<std::string_view>{"prog", "--help"};
  auto result = cli::Parser<HelpFlagArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::help_requested);
  EXPECT_NE(result.error.detail.find("Usage: program [options]"),
            std::string::npos);
  EXPECT_EQ(result.error.message(), result.error.detail);
  EXPECT_TRUE(result.error.use_stdout());
  EXPECT_EQ(result.error.exit_code(), 0);
}

TEST(Help, HelpFlagUsesExplicitProgramNameForSpanParse) {
  auto args = std::vector<std::string_view>{"prog", "--help"};
  auto result = cli::Parser<HelpFlagArgs>{}.parse(
      std::span<const std::string_view>(args), "prog", 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::help_requested);
  EXPECT_NE(result.error.detail.find("Usage: prog [options]"),
            std::string::npos);
}

TEST(Help, HelpFlagUsesProgramNameForPreinitializedArgvParse) {
  char program[] = "prog";
  char help[] = "--help";
  char* argv[] = {program, help};

  auto result = cli::Parser<HelpFlagArgs>{}.parse(HelpFlagArgs{}, 2, argv);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::help_requested);
  EXPECT_NE(result.error.detail.find("Usage: prog [options]"),
            std::string::npos);
}

TEST(Help, CustomHelpFlagNameAndShortOptionWork) {
  auto args = std::vector<std::string_view>{"prog", "-u"};
  auto result = cli::Parser<CustomHelpFlagArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::help_requested);
}

TEST(Help, SubcommandHelpUsesConcatenatedProgramName) {
  auto args = std::vector<std::string_view>{"build", "--help"};
  auto result = cli::Parser<HelpArgs>{}.parse(
      std::span<const std::string_view>(args), "prog", 0);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::help_requested);
  EXPECT_NE(result.error.detail.find("Usage: prog build [options]"),
            std::string::npos);
}

TEST(Help, ExitSuccessActionTriggersDedicatedCode) {
  auto args = std::vector<std::string_view>{"prog", "--version"};
  auto result = cli::Parser<ExitSuccessArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::exit_success);
  EXPECT_EQ(result.error.message(), "");
  EXPECT_TRUE(result.error.use_stdout());
  EXPECT_EQ(result.error.exit_code(), 0);
}

TEST(Help, MetadataIsRenderedAndHiddenFieldsAreOmitted) {
  cli::Parser<MetadataHelpArgs> parser;

  const auto help = parser.format_help(cli::ColorMode::never);
  EXPECT_NE(help.find("--visible"), std::string::npos);
  EXPECT_EQ(help.find("--hidden"), std::string::npos);
  EXPECT_NE(help.find("[deprecated: use --visible instead]"), std::string::npos);
  EXPECT_NE(help.find("[required]"), std::string::npos);
  EXPECT_NE(help.find("[env: APP_OUTPUT]"), std::string::npos);
  EXPECT_NE(help.find("[default: out.txt]"), std::string::npos);
  EXPECT_NE(help.find("legacy"), std::string::npos);
  EXPECT_NE(help.find("[deprecated: use build]"), std::string::npos);
  EXPECT_EQ(help.find("internal"), std::string::npos);
}

TEST(Help, RelationsAreRendered) {
  cli::Parser<MetadataHelpArgs> parser;

  const auto help = parser.format_help(cli::ColorMode::never);
  EXPECT_EQ(help.find("Relations:"), std::string::npos);
  EXPECT_NE(help.find("Option groups:"), std::string::npos);
  EXPECT_NE(help.find("--user"), std::string::npos);
  EXPECT_NE(help.find("legacy_auth:"), std::string::npos);
  EXPECT_NE(help.find("--user, --password must be used together"),
            std::string::npos);
  EXPECT_NE(help.find("[conflicts: --markdown]"), std::string::npos);
  EXPECT_NE(help.find("[conflicts: --json]"), std::string::npos);
  EXPECT_NE(help.find("[requires: --profile]"), std::string::npos);
  EXPECT_NE(help.find("[required by: --deploy]"), std::string::npos);
}
