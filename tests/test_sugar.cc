#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "cli/argument.hh"
#include "cli/parser.hh"

namespace {

struct BuildArgs {
  cli::Flag<"release", 'r'> release{
      {.help = "Release mode", .presence = cli::optional}};
  cli::IntOption<"jobs", 'j'> jobs{
      {.help = "Parallel jobs", .presence = cli::required}};
};

struct SugarArgs {
  cli::Flag<"verbose", 'v'> verbose{
      {.help = "Verbose output", .presence = cli::optional}};
  cli::IntOption<"count", 'c'> count{
      {.help = "Count", .presence = cli::optional}};
  cli::ListOption<std::string, "include", 'I', cli::nargs::exactly<2>> includes{
      {.help = "Include directories", .presence = cli::optional}};
  cli::Positional<std::string, cli::nargs::one_or_more> files{
      {.help = "Input files", .presence = cli::required}};
  cli::Command<"build", BuildArgs> build{{.help = "Build subcommand"}};
};

auto argv(std::initializer_list<std::string_view> values)
    -> std::vector<std::string_view> {
  return {values};
}

using VerboseField =
    std::remove_cvref_t<decltype(std::declval<SugarArgs>().verbose)>;
using CountField =
    std::remove_cvref_t<decltype(std::declval<SugarArgs>().count)>;
using IncludesField =
    std::remove_cvref_t<decltype(std::declval<SugarArgs>().includes)>;
using FilesField =
    std::remove_cvref_t<decltype(std::declval<SugarArgs>().files)>;

static_assert(std::same_as<VerboseField::value_type, bool>);
static_assert(std::same_as<CountField::value_type, int>);
static_assert(std::same_as<IncludesField::value_type, std::vector<std::string>>);
static_assert(std::same_as<FilesField::value_type, std::vector<std::string>>);
static_assert(
    std::same_as<cli::Positional<std::string>::value_type, std::string>);
static_assert(
    std::same_as<cli::ListOption<int, "ports">::value_type, std::vector<int>>);

}  // namespace

TEST(Sugar, ParsesConvenienceAliases) {
  auto args =
      argv({"prog", "--verbose", "--count", "3", "--include", "inc/a", "inc/b",
            "main.cc", "util.cc", "build", "--jobs", "8", "--release"});

  auto result =
      cli::Parser<SugarArgs>{}.parse(std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_TRUE(result.value.verbose.value());
  ASSERT_TRUE(result.value.count);
  EXPECT_EQ(*result.value.count, 3);
  EXPECT_EQ((result.value.includes.value()),
            (std::vector<std::string>{"inc/a", "inc/b"}));
  EXPECT_EQ((result.value.files.value()),
            (std::vector<std::string>{"main.cc", "util.cc"}));
  EXPECT_TRUE(result.value.build.provided());
  EXPECT_TRUE(result.value.build.release.value());
  ASSERT_TRUE(result.value.build.jobs);
  EXPECT_EQ(*result.value.build.jobs, 8);
}

TEST(Sugar, SinglePositionalStoresValue) {
  struct SinglePositionalArgs {
    cli::Positional<std::string> file{
        {.help = "Input file", .presence = cli::required}};
  };

  auto args = argv({"prog", "input.txt"});
  auto result = cli::Parser<SinglePositionalArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result.value.file);
  EXPECT_EQ(*result.value.file, "input.txt");
}

TEST(Sugar, RequiredAliasStillEnforcesPresence) {
  struct RequiredArgs {
    cli::IntOption<"port", 'p'> port{
        {.help = "Port", .presence = cli::required}};
  };

  auto args = argv({"prog"});
  auto result = cli::Parser<RequiredArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::missing_required);
  EXPECT_EQ(result.error.subject, "--port");
}

TEST(Sugar, DefaultMissingValueSupportsFlagAndExplicitValue) {
  struct DefaultArgs {
    cli::Arg<"mode",
             cli::conversion::default_missing_value<"auto"> |
                 cli::conversion::string | cli::pack::set_once,
             cli::nargs::zero_or_one>
        mode{{.help = "Mode", .presence = cli::Presence::optional}};
  };

  {
    auto args = argv({"prog", "--mode"});
    auto result = cli::Parser<DefaultArgs>{}.parse(
        std::span<const std::string_view>(args), 1);

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result.value.mode);
    EXPECT_EQ(*result.value.mode, "auto");
  }

  {
    auto args = argv({"prog", "--mode", "manual"});
    auto result = cli::Parser<DefaultArgs>{}.parse(
        std::span<const std::string_view>(args), 1);

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result.value.mode);
    EXPECT_EQ(*result.value.mode, "manual");
  }
}

TEST(Sugar, ValueOptionExposesOptionalLikeApi) {
  struct DefaultArgs {
    cli::IntOption<"jobs", 'j'> jobs{{.default_value = 4}};
    cli::StringOption<"name", 'n'> name;
  };

  auto args = argv({"prog"});
  auto result = cli::Parser<DefaultArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result.value.jobs.provided());
  EXPECT_EQ(result.value.jobs.occurrences(), 0U);
  EXPECT_TRUE(result.value.jobs);
  EXPECT_EQ(*result.value.jobs, 4);
  EXPECT_EQ(result.value.jobs.value_or(1), 4);
  auto as_optional = static_cast<std::optional<int>>(result.value.jobs);
  ASSERT_TRUE(as_optional.has_value());
  EXPECT_EQ(*as_optional, 4);

  EXPECT_FALSE(result.value.name);
  EXPECT_EQ(result.value.name.value_or("fallback"), "fallback");
  auto missing = static_cast<std::optional<std::string>>(result.value.name);
  EXPECT_FALSE(missing.has_value());
}
