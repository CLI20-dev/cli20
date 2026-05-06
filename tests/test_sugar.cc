#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include "argon/argument.hh"
#include "argon/parser.hh"

namespace {

struct BuildArgs {
  argon::FlagOption<"release", 'r'> release{
      {.help = "Release mode", .presence = argon::optional}};
  argon::IntOption<"jobs", 'j'> jobs{
      {.help = "Parallel jobs", .presence = argon::required}};
};

struct SugarArgs {
  argon::FlagOption<"verbose", 'v'> verbose{
      {.help = "Verbose output", .presence = argon::optional}};
  argon::IntOption<"count", 'c'> count{
      {.help = "Count", .presence = argon::optional}};
  argon::StringListOption<"include", 'I', argon::nargs::exactly<2>> includes{
      {.help = "Include directories", .presence = argon::optional}};
  argon::Positional<std::string, argon::nargs::one_or_more> files{
      {.help = "Input files", .presence = argon::required}};
  argon::Command<"build", BuildArgs> build{{.help = "Build subcommand"}};
};

auto argv(std::initializer_list<std::string_view> values)
    -> std::vector<std::string_view> {
  return {values};
}

using VerboseField = std::remove_cvref_t<decltype(std::declval<SugarArgs>().verbose)>;
using CountField = std::remove_cvref_t<decltype(std::declval<SugarArgs>().count)>;
using IncludesField =
    std::remove_cvref_t<decltype(std::declval<SugarArgs>().includes)>;
using FilesField = std::remove_cvref_t<decltype(std::declval<SugarArgs>().files)>;

static_assert(std::same_as<VerboseField::value_type, bool>);
static_assert(std::same_as<CountField::value_type, std::optional<int>>);
static_assert(std::same_as<IncludesField::value_type, std::vector<std::string>>);
static_assert(std::same_as<FilesField::value_type, std::vector<std::string>>);
static_assert(std::same_as<argon::StringPositional::value_type,
                           std::optional<std::string>>);
static_assert(std::same_as<argon::IntListArg<"ports">::value_type,
                           std::vector<int>>);

}  // namespace

TEST(Sugar, ParsesConvenienceAliases) {
  auto args = argv({"prog", "--verbose", "--count", "3", "--include", "inc/a",
                    "inc/b", "main.cc", "util.cc", "build", "--jobs", "8",
                    "--release"});

  auto result = argon::Parser<SugarArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_TRUE(result.value.verbose.value());
  ASSERT_TRUE(result.value.count.value().has_value());
  EXPECT_EQ(*result.value.count.value(), 3);
  EXPECT_EQ((result.value.includes.value()),
            (std::vector<std::string>{"inc/a", "inc/b"}));
  EXPECT_EQ((result.value.files.value()),
            (std::vector<std::string>{"main.cc", "util.cc"}));
  EXPECT_TRUE(result.value.build.provided());
  EXPECT_TRUE(result.value.build.release.value());
  ASSERT_TRUE(result.value.build.jobs.value().has_value());
  EXPECT_EQ(*result.value.build.jobs.value(), 8);
}

TEST(Sugar, SinglePositionalStoresOptionalValue) {
  struct SinglePositionalArgs {
    argon::StringPositional file{
        {.help = "Input file", .presence = argon::required}};
  };

  auto args = argv({"prog", "input.txt"});
  auto result = argon::Parser<SinglePositionalArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result.value.file.value().has_value());
  EXPECT_EQ(*result.value.file.value(), "input.txt");
}

TEST(Sugar, RequiredAliasStillEnforcesPresence) {
  struct RequiredArgs {
    argon::IntOption<"port", 'p'> port{
        {.help = "Port", .presence = argon::required}};
  };

  auto args = argv({"prog"});
  auto result = argon::Parser<RequiredArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, argon::ErrorCode::missing_required);
  EXPECT_EQ(result.error.subject, "port");
}
