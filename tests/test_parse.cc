#include <gtest/gtest.h>

#include <string_view>
#include <vector>

#include "cli/argument.hh"
#include "cli/parser.hh"

namespace {

struct BuildArgs {
  cli::IntOption<"threads", 't'> threads{
      {.help = "Worker threads", .presence = cli::Presence::required}};
};

struct ParseArgs {
  cli::Flag<"verbose", 'v'> verbose{
      {.help = "Verbose output", .presence = cli::Presence::optional}};

  cli::IntOption<"count", 'c'> count{
      {.help = "Positive count", .presence = cli::Presence::required}};

  cli::Positional<std::string, cli::nargs::one_or_more> files{
      {.help = "Input files", .presence = cli::Presence::required}};

  cli::Command<"build", BuildArgs> build{{.help = "Build subcommand"}};
};

auto argv(std::initializer_list<std::string_view> values)
    -> std::vector<std::string_view> {
  return {values};
}

}  // namespace

TEST(Parse, ParsesOptionsPositionalsAndSubcommand) {
  auto args = argv({"prog", "--verbose", "--count", "2", "a.txt", "b.txt",
                    "build", "--threads", "4"});

  auto result =
      cli::Parser<ParseArgs>{}.parse(std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value.verbose.value());
  ASSERT_TRUE(result.value.count.value().has_value());
  EXPECT_EQ(*result.value.count.value(), 2);
  EXPECT_EQ((result.value.files.value()),
            (std::vector<std::string>{"a.txt", "b.txt"}));
  EXPECT_TRUE(result.value.build.provided());
  ASSERT_TRUE(result.value.build.threads.value().has_value());
  EXPECT_EQ(*result.value.build.threads.value(), 4);
}

TEST(Parse, MissingRequiredOptionFails) {
  auto args = argv({"prog", "file.txt"});

  auto result =
      cli::Parser<ParseArgs>{}.parse(std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::missing_required);
  EXPECT_EQ(result.error.subject, "count");
}

TEST(Parse, DuplicateSetOnceFails) {
  auto args = argv({"prog", "--count", "1", "--count", "2", "file.txt"});

  auto result =
      cli::Parser<ParseArgs>{}.parse(std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::duplicate_argument);
  EXPECT_EQ(result.error.position, 4);
}

TEST(Parse, InvalidIntegerReportsAbsoluteArgvPosition) {
  auto args = argv({"prog", "--count", "12ms", "file.txt"});

  auto result =
      cli::Parser<ParseArgs>{}.parse(std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::invalid_value);
  EXPECT_EQ(result.error.position, 2);
  EXPECT_EQ(result.error.detail, "unexpected trailing characters");
}

TEST(Parse, MissingRequiredSubcommandOptionFailsInsideCommand) {
  auto args = argv({"prog", "--count", "1", "file.txt", "build"});

  auto result =
      cli::Parser<ParseArgs>{}.parse(std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::missing_required);
  EXPECT_EQ(result.error.subject, "threads");
}
