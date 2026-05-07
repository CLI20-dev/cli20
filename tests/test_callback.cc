#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include "cli/argument.hh"
#include "cli/parser.hh"

namespace {

auto argv(std::initializer_list<std::string_view> values)
    -> std::vector<std::string_view> {
  return {values};
}

struct CallbackIntArgs {
  cli::IntOption<"count", 'c'> count;
};

struct CallbackFlagArgs {
  cli::Flag<"verbose", 'v'> verbose;
};

struct CallbackListArgs {
  cli::ListOption<std::string, "file", 'f'> files;
};

struct CallbackPositionalArgs {
  cli::Positional<std::string, cli::nargs::one_or_more> inputs;
};

}  // namespace

TEST(Callback, CalledForIntOption) {
  int captured = -1;
  CallbackIntArgs args;
  args.count = cli::IntOption<"count", 'c'>{{
      .on_parse =
          [&captured](const std::optional<int>& v) {
            if (v) captured = *v;
          },
  }};

  auto args_vec = argv({"prog", "--count", "42"});
  auto result = cli::Parser<CallbackIntArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_EQ(captured, 42);
}

TEST(Callback, CalledForFlag) {
  bool fired = false;
  CallbackFlagArgs args;
  args.verbose = cli::Flag<"verbose", 'v'>{{
      .on_parse = [&fired](const bool& v) { fired = v; },
  }};

  auto args_vec = argv({"prog", "--verbose"});
  auto result = cli::Parser<CallbackFlagArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_TRUE(fired);
}

TEST(Callback, NotCalledWhenFlagAbsent) {
  bool fired = false;
  CallbackFlagArgs args;
  args.verbose = cli::Flag<"verbose", 'v'>{{
      .on_parse = [&fired](const bool& v) { fired = v; },
  }};

  auto args_vec = argv({"prog"});
  auto result = cli::Parser<CallbackFlagArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_FALSE(fired);
}

TEST(Callback, CalledPerElementForList) {
  std::vector<std::string> snapshots;
  CallbackListArgs args;
  args.files = cli::ListOption<std::string, "file", 'f'>{{
      .on_parse =
          [&snapshots](const std::vector<std::string>& v) {
            snapshots.push_back(
                v.back());  // capture each new element as it arrives
          },
  }};

  auto args_vec =
      argv({"prog", "--file", "a.txt", "--file", "b.txt", "--file", "c.txt"});
  auto result = cli::Parser<CallbackListArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_EQ(snapshots, (std::vector<std::string>{"a.txt", "b.txt", "c.txt"}));
}

TEST(Callback, CalledForPositional) {
  std::vector<std::string> seen;
  CallbackPositionalArgs args;
  args.inputs = cli::Positional<std::string, cli::nargs::one_or_more>{{
      .on_parse =
          [&seen](const std::vector<std::string>& v) {
            seen.push_back(v.back());
          },
  }};

  auto args_vec = argv({"prog", "foo.cc", "bar.cc"});
  auto result = cli::Parser<CallbackPositionalArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_EQ(seen, (std::vector<std::string>{"foo.cc", "bar.cc"}));
}

TEST(Callback, NotCalledOnParseError) {
  int call_count = 0;
  CallbackIntArgs args;
  args.count = cli::IntOption<"count", 'c'>{{
      .on_parse = [&call_count](const std::optional<int>&) { ++call_count; },
  }};

  auto args_vec = argv({"prog", "--count", "notanint"});
  auto result = cli::Parser<CallbackIntArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(call_count, 0);
}

TEST(Callback, CallbackCountMatchesParseCount) {
  int call_count = 0;
  CallbackListArgs args;
  args.files = cli::ListOption<std::string, "file", 'f'>{{
      .on_parse =
          [&call_count](const std::vector<std::string>&) { ++call_count; },
  }};

  auto args_vec = argv({"prog", "--file", "a", "--file", "b", "--file", "c"});
  auto result = cli::Parser<CallbackListArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_EQ(call_count, 3);
}
