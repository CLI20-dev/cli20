#include <gtest/gtest.h>

#include <string_view>
#include <vector>

#include "argon/argument.hh"
#include "argon/color.hh"
#include "argon/parser.hh"

namespace {

struct BuildArgs {
  argon::Description description{"Compile sources into an executable."};

  argon::FlagOption<"release", 'r'> release{
      {.help = "Build with optimizations", .presence = argon::optional}};
  argon::IntOption<"jobs", 'j'> jobs{
      {.help = "Parallel job count", .presence = argon::required}};
};

struct HelpArgs {
  argon::Description description{
      "A small tool used to exercise the help generator."};

  argon::FlagOption<"verbose", 'v'> verbose{
      {.help = "Enable verbose logging", .presence = argon::optional}};
  argon::IntOption<"count", 'c'> count{
      {.help = "Number of iterations", .presence = argon::optional}};
  argon::StringListOption<"include", 'I', argon::nargs::exactly<2>> includes{
      {.help = "Two include directories", .presence = argon::optional}};
  argon::StringPositional input{
      {.help = "Input file", .presence = argon::required}};
  argon::Command<"build", BuildArgs> build{{.help = "Run the build step"}};
};

struct OptionsOnlyArgs {
  argon::FlagOption<"verbose", 'v'> verbose{
      {.help = "Enable verbose logging", .presence = argon::optional}};
};

struct CommandsOnlyArgs {
  argon::Command<"build", BuildArgs> build{{.help = "Run the build step"}};
};

struct HelpFlagArgs {
  argon::HelpFlag<> help;
};

struct CustomHelpFlagArgs {
  argon::HelpFlag<"usage", 'u'> usage;
};

struct ExitSuccessArgs {
  argon::ArgImpl<"version", 'V', argon::nargs::none,
                 argon::Action<argon::action::exit_success>{}>
      version{{.help = "Print version and exit", .presence = argon::optional}};
};

}  // namespace

TEST(Help, UsageContainsProgramAndMajorGroups) {
  char program[] = "myprog";
  char* argv[] = {program};

  argon::Parser<HelpArgs> parser;
  std::ignore = parser.parse(1, argv);

  const auto help = parser.formatHelp(argon::ColorMode::never);
  EXPECT_TRUE(help.starts_with("Usage: myprog"));
  EXPECT_NE(help.find("[options]"), std::string::npos);
  EXPECT_NE(help.find("[args]"), std::string::npos);
  EXPECT_NE(help.find("[command]"), std::string::npos);
}

TEST(Help, DescriptionAndSectionsAppear) {
  argon::Parser<HelpArgs> parser;

  const auto help = parser.formatHelp(argon::ColorMode::never);
  EXPECT_NE(help.find("A small tool used to exercise the help generator."),
            std::string::npos);
  EXPECT_NE(help.find("Options:"), std::string::npos);
  EXPECT_NE(help.find("Positional arguments:"), std::string::npos);
  EXPECT_NE(help.find("Commands:"), std::string::npos);
}

TEST(Help, OptionAndPositionalMetavarsAreRendered) {
  argon::Parser<HelpArgs> parser;

  const auto help = parser.formatHelp(argon::ColorMode::never);
  EXPECT_NE(help.find("-c, --count <int>"), std::string::npos);
  EXPECT_NE(help.find("-I, --include <string...>"), std::string::npos);
  EXPECT_NE(help.find("<string>"), std::string::npos);
}

TEST(Help, FlagDoesNotShowMetavar) {
  argon::Parser<HelpArgs> parser;

  const auto help = parser.formatHelp(argon::ColorMode::never);
  const auto line_start = help.find("-v, --verbose");
  ASSERT_NE(line_start, std::string::npos);
  const auto line_end = help.find('\n', line_start);
  const auto line = help.substr(line_start, line_end - line_start);
  EXPECT_EQ(line.find("<bool>"), std::string::npos);
}

TEST(Help, EmptySectionsAreOmitted) {
  argon::Parser<OptionsOnlyArgs> options_only;
  const auto options_help = options_only.formatHelp(argon::ColorMode::never);
  EXPECT_NE(options_help.find("Options:"), std::string::npos);
  EXPECT_EQ(options_help.find("Positional arguments:"), std::string::npos);
  EXPECT_EQ(options_help.find("Commands:"), std::string::npos);

  argon::Parser<CommandsOnlyArgs> commands_only;
  const auto commands_help = commands_only.formatHelp(argon::ColorMode::never);
  EXPECT_EQ(commands_help.find("Options:"), std::string::npos);
  EXPECT_EQ(commands_help.find("Positional arguments:"), std::string::npos);
  EXPECT_NE(commands_help.find("Commands:"), std::string::npos);
}

TEST(Help, RecursiveHelpIncludesSubcommandBody) {
  argon::Parser<HelpArgs> parser;

  const auto help = parser.formatHelp(argon::ColorMode::never, argon::recurseHelp);
  EXPECT_NE(help.find("Usage: program build [options]"), std::string::npos);
  EXPECT_NE(help.find("Compile sources into an executable."), std::string::npos);
  EXPECT_NE(help.find("-j, --jobs <int>"), std::string::npos);
}

TEST(Help, ColorNeverDoesNotEmitAnsi) {
  argon::Parser<HelpArgs> parser;

  const auto help = parser.formatHelp(argon::ColorMode::never);
  EXPECT_EQ(help.find("\033["), std::string::npos);
}

TEST(Help, ColorAlwaysEmitsAnsiHeadingsAndLabels) {
  argon::Parser<HelpArgs> parser;

  const auto help = parser.formatHelp(argon::ColorMode::always);
  EXPECT_NE(help.find("\033[1m"), std::string::npos);
  EXPECT_NE(help.find("\033[4m"), std::string::npos);
  EXPECT_NE(help.find("\033[0m"), std::string::npos);
  EXPECT_NE(help.find("\033[1m-v, --verbose"), std::string::npos);
}

TEST(Help, ResolveColorRespectsMode) {
  const auto never = argon::detail::resolveColor(argon::ColorMode::never);
  const auto always = argon::detail::resolveColor(argon::ColorMode::always);

  EXPECT_EQ(never.bold(), "");
  EXPECT_EQ(never.underline(), "");
  EXPECT_EQ(never.reset(), "");

  EXPECT_EQ(always.bold(), "\033[1m");
  EXPECT_EQ(always.underline(), "\033[4m");
  EXPECT_EQ(always.reset(), "\033[0m");
}

TEST(Help, HelpFlagTriggersHelpRequested) {
  auto args = std::vector<std::string_view>{"prog", "--help"};
  auto result = argon::Parser<HelpFlagArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, argon::ErrorCode::help_requested);
  EXPECT_NE(result.error.detail.find("Usage: program [options]"), std::string::npos);
  EXPECT_EQ(result.error.message(), result.error.detail);
  EXPECT_TRUE(result.error.useStdout());
  EXPECT_EQ(result.error.exitCode(), 0);
}

TEST(Help, CustomHelpFlagNameAndShortOptionWork) {
  auto args = std::vector<std::string_view>{"prog", "-u"};
  auto result = argon::Parser<CustomHelpFlagArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, argon::ErrorCode::help_requested);
}

TEST(Help, ExitSuccessActionTriggersDedicatedCode) {
  auto args = std::vector<std::string_view>{"prog", "--version"};
  auto result = argon::Parser<ExitSuccessArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, argon::ErrorCode::exit_success);
  EXPECT_EQ(result.error.message(), "");
  EXPECT_TRUE(result.error.useStdout());
  EXPECT_EQ(result.error.exitCode(), 0);
}
