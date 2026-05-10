#include <gtest/gtest.h>

#include <cstdlib>
#include <string_view>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static auto setenv(const char* name, const char* value, int) -> int {
  return SetEnvironmentVariableA(name, value) ? 0 : -1;
}
static auto unsetenv(const char* name) -> void {
  SetEnvironmentVariableA(name, nullptr);
}
#endif

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

#ifdef _WIN32
namespace {

// Build a wchar_t* argv[] from a list of wide string literals for testing wmain.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
auto wide_argv(std::initializer_list<const wchar_t*> values)
    -> std::vector<wchar_t*> {
  std::vector<wchar_t*> result;
  result.reserve(values.size());
  for (const wchar_t* p : values) {
    result.push_back(const_cast<wchar_t*>(p));
  }
  return result;
}

struct UnicodeArgs {
  cli::StringOption<"name", 'n'> name{
      {.help = "Name", .presence = cli::Presence::required}};
  cli::Positional<std::string, cli::nargs::zero_or_more> files{
      {.help = "Files", .presence = cli::Presence::optional}};
};

}  // namespace

// ASCII via wide argv — basic sanity check.
TEST(ParseWide, AsciiRoundtrip) {
  auto args = wide_argv({L"prog", L"--name", L"hello"});
  auto result = cli::Parser<UnicodeArgs>{}.parse(static_cast<int>(args.size()),
                                                 args.data());

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result.value.name.value().has_value());
  EXPECT_EQ(*result.value.name.value(), "hello");
}

// Japanese: テスト (U+30C6 U+30B9 U+30C8) — 3-byte UTF-8 sequences.
TEST(ParseWide, JapaneseOption) {
  auto args = wide_argv({L"prog", L"--name", L"\u30C6\u30B9\u30C8"});
  auto result = cli::Parser<UnicodeArgs>{}.parse(static_cast<int>(args.size()),
                                                 args.data());

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result.value.name.value().has_value());
  EXPECT_EQ(*result.value.name.value(), "\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88");
}

// Chinese: 你好 (U+4F60 U+597D).
TEST(ParseWide, ChineseOption) {
  auto args = wide_argv({L"prog", L"--name", L"\u4F60\u597D"});
  auto result = cli::Parser<UnicodeArgs>{}.parse(static_cast<int>(args.size()),
                                                 args.data());

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result.value.name.value().has_value());
  EXPECT_EQ(*result.value.name.value(), "\xe4\xbd\xa0\xe5\xa5\xbd");
}

// Emoji: 😀 (U+1F600) — 4-byte UTF-8 / surrogate pair in UTF-16.
TEST(ParseWide, EmojiOption) {
  auto args = wide_argv({L"prog", L"--name", L"\U0001F600"});
  auto result = cli::Parser<UnicodeArgs>{}.parse(static_cast<int>(args.size()),
                                                 args.data());

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result.value.name.value().has_value());
  EXPECT_EQ(*result.value.name.value(), "\xf0\x9f\x98\x80");
}

// Unicode in positional arguments.
TEST(ParseWide, UnicodePositionals) {
  auto args = wide_argv(
      {L"prog", L"--name", L"x", L"\u30A2\u30A4\u30A6", L"\u6587\u4EF6"});
  auto result = cli::Parser<UnicodeArgs>{}.parse(static_cast<int>(args.size()),
                                                 args.data());

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value.files.value(),
            (std::vector<std::string>{
                "\xe3\x82\xa2\xe3\x82\xa4\xe3\x82\xa6",  // アイウ
                "\xe6\x96\x87\xe4\xbb\xb6",              // 文件
            }));
}

// Unicode program name is stored correctly.
TEST(ParseWide, UnicodeProgramName) {
  auto args = wide_argv({L"\u30A2\u30D7\u30EA", L"--name", L"x"});
  cli::Parser<UnicodeArgs> parser;
  auto result = parser.parse(static_cast<int>(args.size()), args.data());

  ASSERT_TRUE(result.has_value());
  // program_name_ is not directly exposed, but parse succeeds without
  // corruption.
}

// Error path: missing required option still works through wide argv.
TEST(ParseWide, MissingRequiredOptionFails) {
  auto args = wide_argv({L"prog"});
  auto result = cli::Parser<UnicodeArgs>{}.parse(static_cast<int>(args.size()),
                                                 args.data());

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::missing_required);
  EXPECT_EQ(result.error.subject, "name");
}

// Free-function parse() overload for wchar_t*.
TEST(ParseWide, FreeFunctionParse) {
  auto args = wide_argv({L"prog", L"--name", L"\u30C6\u30B9\u30C8"});
  auto result =
      cli::parse<UnicodeArgs>(static_cast<int>(args.size()), args.data());

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result.value.name.value().has_value());
  EXPECT_EQ(*result.value.name.value(), "\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88");
}
#endif

// ---- short cluster / attached value (end-to-end) --------------------------

namespace {

struct ClusterArgs {
  cli::Flag<"verbose", 'v'> verbose;
  cli::Flag<"xray", 'x'> xray;
  cli::Flag<"force", 'f'> force;
  cli::StringOption<"output", 'o'> output;
  cli::IntOption<"count", 'c'> count;
};

}  // namespace

TEST(ParseCluster, AllFlagsCluster) {
  auto args = argv({"prog", "-vxf"});
  auto result = cli::Parser<ClusterArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value.verbose.value());
  EXPECT_TRUE(result.value.xray.value());
  EXPECT_TRUE(result.value.force.value());
}

TEST(ParseCluster, AttachedValue) {
  // -ofile  →  output = "file"
  auto args = argv({"prog", "-ofile"});
  auto result = cli::Parser<ClusterArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result.value.output.value().has_value());
  EXPECT_EQ(*result.value.output.value(), "file");
}

TEST(ParseCluster, FlagsThenAttachedValue) {
  // -vxofile  →  verbose, xray, output="file"
  auto args = argv({"prog", "-vxofile"});
  auto result = cli::Parser<ClusterArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value.verbose.value());
  EXPECT_TRUE(result.value.xray.value());
  ASSERT_TRUE(result.value.output.value().has_value());
  EXPECT_EQ(*result.value.output.value(), "file");
}

TEST(ParseCluster, FlagsThenValueNextToken) {
  // -vxo out.txt  →  verbose, xray, output="out.txt"
  auto args = argv({"prog", "-vxo", "out.txt"});
  auto result = cli::Parser<ClusterArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value.verbose.value());
  EXPECT_TRUE(result.value.xray.value());
  ASSERT_TRUE(result.value.output.value().has_value());
  EXPECT_EQ(*result.value.output.value(), "out.txt");
}

// ---- cluster with nargs=2 option -------------------------------------------

namespace {

struct ClusterNargs2Args {
  cli::Flag<"verbose", 'v'> verbose;
  cli::Flag<"xray", 'x'> xray;
  cli::ListOption<std::string, "output", 'o', cli::nargs::exactly<2>> output;
  cli::Positional<std::string, cli::nargs::zero_or_more> files;
};

}  // namespace

TEST(ParseClusterNargs2, AttachedFirstValueThenNextToken) {
  // -ofile1 file2 file3  →  output={"file1","file2"}, files={"file3"}
  auto args = argv({"prog", "-ofile1", "file2", "file3"});
  auto result = cli::Parser<ClusterNargs2Args>{}.parse(
      std::span<const std::string_view>(args), 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value.output.value(),
            (std::vector<std::string>{"file1", "file2"}));
  EXPECT_EQ(result.value.files.value(), (std::vector<std::string>{"file3"}));
}

TEST(ParseClusterNargs2, FlagsThenValueNextTokens) {
  // -vxo file1 file2 file3  →  verbose, xray, output={"file1","file2"},
  // files={"file3"}
  auto args = argv({"prog", "-vxo", "file1", "file2", "file3"});
  auto result = cli::Parser<ClusterNargs2Args>{}.parse(
      std::span<const std::string_view>(args), 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value.verbose.value());
  EXPECT_TRUE(result.value.xray.value());
  EXPECT_EQ(result.value.output.value(),
            (std::vector<std::string>{"file1", "file2"}));
  EXPECT_EQ(result.value.files.value(), (std::vector<std::string>{"file3"}));
}

// ---- env fallback ----------------------------------------------------------

namespace {

struct EnvArgs {
  cli::StringOption<"output", 'o'> output{{.env = "TEST_OUTPUT"}};
  cli::IntOption<"count", 'c'> count{{.env = "TEST_COUNT"}};
  cli::Flag<"verbose", 'v'> verbose{{.env = "TEST_VERBOSE"}};
};

}  // namespace

TEST(ParseEnv, FallsBackToEnvWhenOptionAbsent) {
  setenv("TEST_OUTPUT", "from_env", 1);
  auto args = argv({"prog"});
  auto result =
      cli::Parser<EnvArgs>{}.parse(std::span<const std::string_view>(args), 1);
  unsetenv("TEST_OUTPUT");
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result.value.output.value().has_value());
  EXPECT_EQ(*result.value.output.value(), "from_env");
}

TEST(ParseEnv, CliTakesPrecedenceOverEnv) {
  setenv("TEST_OUTPUT", "from_env", 1);
  auto args = argv({"prog", "--output", "from_cli"});
  auto result =
      cli::Parser<EnvArgs>{}.parse(std::span<const std::string_view>(args), 1);
  unsetenv("TEST_OUTPUT");
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result.value.output.value().has_value());
  EXPECT_EQ(*result.value.output.value(), "from_cli");
}

TEST(ParseEnv, EnvSatisfiesRequiredOption) {
  struct RequiredEnvArgs {
    cli::StringOption<"output"> output{
        {.presence = cli::required, .env = "TEST_OUTPUT"}};
  };
  setenv("TEST_OUTPUT", "env_val", 1);
  auto args = argv({"prog"});
  auto result = cli::Parser<RequiredEnvArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  unsetenv("TEST_OUTPUT");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result.value.output.value(), "env_val");
}

TEST(ParseEnv, MissingEnvStillFailsForRequired) {
  struct RequiredEnvArgs {
    cli::StringOption<"output"> output{
        {.presence = cli::required, .env = "TEST_OUTPUT_ABSENT"}};
  };
  unsetenv("TEST_OUTPUT_ABSENT");
  auto args = argv({"prog"});
  auto result = cli::Parser<RequiredEnvArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::missing_required);
}

TEST(ParseEnv, IntOptionFromEnv) {
  setenv("TEST_COUNT", "42", 1);
  auto args = argv({"prog"});
  auto result =
      cli::Parser<EnvArgs>{}.parse(std::span<const std::string_view>(args), 1);
  unsetenv("TEST_COUNT");
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result.value.count.value().has_value());
  EXPECT_EQ(*result.value.count.value(), 42);
}

TEST(ParseEnv, FlagFromEnv) {
  setenv("TEST_VERBOSE", "1", 1);
  auto args = argv({"prog"});
  auto result =
      cli::Parser<EnvArgs>{}.parse(std::span<const std::string_view>(args), 1);
  unsetenv("TEST_VERBOSE");
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value.verbose.value());
}

TEST(ParseEnv, InvalidEnvValueReportsError) {
  setenv("TEST_COUNT", "not_a_number", 1);
  auto args = argv({"prog"});
  auto result =
      cli::Parser<EnvArgs>{}.parse(std::span<const std::string_view>(args), 1);
  unsetenv("TEST_COUNT");
  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::invalid_value);
}
