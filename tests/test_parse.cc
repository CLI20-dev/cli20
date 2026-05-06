#include <gtest/gtest.h>

#include <string_view>
#include <vector>

#include "argon/argument.hh"
#include "argon/parser.hh"

namespace {

struct BuildArgs {
  argon::ArgImpl<
      "threads", 't', {.min = 1, .max = 1},
      argon::Action<argon::conversion::integer<int>, argon::validation::positive,
                    argon::pack::set_once>{}>
      threads{{.help = "Worker threads", .presence = argon::Presence::required}};
};

struct ParseArgs {
  argon::ArgImpl<"verbose", 'v', {.min = 0, .max = 0},
                 argon::Action<argon::pack::set_true>{}>
      verbose{{.help = "Verbose output", .presence = argon::Presence::optional}};

  argon::ArgImpl<
      "count", 'c', {.min = 1, .max = 1},
      argon::Action<argon::conversion::integer<int>, argon::validation::positive,
                    argon::pack::set_once>{}>
      count{{.help = "Positive count", .presence = argon::Presence::required}};

  argon::PositionalImpl<
      {.min = 1, .max = -1},
      argon::Action<argon::conversion::string, argon::validation::not_blank,
                    argon::pack::push>{}>
      files{{.help = "Input files", .presence = argon::Presence::required}};

  argon::Command<"build", BuildArgs> build{{.help = "Build subcommand"}};
};

auto argv(std::initializer_list<std::string_view> values)
    -> std::vector<std::string_view> {
  return {values};
}

}  // namespace

TEST(Parse, ParsesOptionsPositionalsAndSubcommand) {
  auto args = argv({"prog", "--verbose", "--count", "2", "a.txt", "b.txt",
                    "build", "--threads", "4"});

  auto result = argon::Parser<ParseArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

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

  auto result = argon::Parser<ParseArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, argon::ErrorCode::missing_required);
  EXPECT_EQ(result.error.subject, "count");
}

TEST(Parse, DuplicateSetOnceFails) {
  auto args = argv({"prog", "--count", "1", "--count", "2", "file.txt"});

  auto result = argon::Parser<ParseArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, argon::ErrorCode::duplicate_argument);
  EXPECT_EQ(result.error.position, 4);
}

TEST(Parse, InvalidIntegerReportsAbsoluteArgvPosition) {
  auto args = argv({"prog", "--count", "12ms", "file.txt"});

  auto result = argon::Parser<ParseArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, argon::ErrorCode::invalid_value);
  EXPECT_EQ(result.error.position, 2);
  EXPECT_EQ(result.error.detail, "unexpected trailing characters");
}

TEST(Parse, MissingRequiredSubcommandOptionFailsInsideCommand) {
  auto args = argv({"prog", "--count", "1", "file.txt", "build"});

  auto result = argon::Parser<ParseArgs>{}.parse(
      std::span<const std::string_view>(args), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, argon::ErrorCode::missing_required);
  EXPECT_EQ(result.error.subject, "threads");
}
