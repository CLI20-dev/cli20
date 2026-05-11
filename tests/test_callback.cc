#include <gtest/gtest.h>

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
      .on_parse = [&captured](const int& v) -> void { captured = v; },
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
      .on_parse = [&fired](const bool& v) -> void { fired = v; },
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
      .on_parse = [&fired](const bool& v) -> void { fired = v; },
  }};

  auto args_vec = argv({"prog"});
  auto result = cli::Parser<CallbackFlagArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_FALSE(fired);
}

// on_parse fires once per option *occurrence*, not once per value token.
// --file a.txt b.txt c.txt  (one occurrence, 3 values) → fires once.
TEST(Callback, CalledOncePerOccurrenceForList) {
  int call_count = 0;
  std::vector<std::string> last_seen;
  CallbackListArgs args;
  args.files = cli::ListOption<std::string, "file", 'f'>{{
      .on_parse = [&](const std::vector<std::string>& v) -> void {
        ++call_count;
        last_seen = v;
      },
  }};

  // One occurrence of --file with three values.
  auto args_vec = argv({"prog", "--file", "a.txt", "b.txt", "c.txt"});
  auto result = cli::Parser<CallbackListArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_EQ(call_count, 1);
  EXPECT_EQ(last_seen, (std::vector<std::string>{"a.txt", "b.txt", "c.txt"}));
}

// on_parse for positionals fires once after all tokens are consumed, not per
// token.
TEST(Callback, CalledOnceForPositional) {
  int call_count = 0;
  std::vector<std::string> final_value;
  CallbackPositionalArgs args;
  args.inputs = cli::Positional<std::string, cli::nargs::one_or_more>{{
      .on_parse = [&](const std::vector<std::string>& v) -> void {
        ++call_count;
        final_value = v;
      },
  }};

  auto args_vec = argv({"prog", "foo.cc", "bar.cc", "baz.cc"});
  auto result = cli::Parser<CallbackPositionalArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_EQ(call_count, 1);
  EXPECT_EQ(final_value,
            (std::vector<std::string>{"foo.cc", "bar.cc", "baz.cc"}));
}

TEST(Callback, NotCalledOnParseError) {
  int call_count = 0;
  CallbackIntArgs args;
  args.count = cli::IntOption<"count", 'c'>{{
      .on_parse = [&call_count](const int&) -> void { ++call_count; },
  }};

  auto args_vec = argv({"prog", "--count", "notanint"});
  auto result = cli::Parser<CallbackIntArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(call_count, 0);
}

// Three separate occurrences of --file (each with one value) → fires 3 times.
TEST(Callback, CallbackCountMatchesOccurrenceCount) {
  int call_count = 0;
  CallbackListArgs args;
  args.files = cli::ListOption<std::string, "file", 'f'>{{
      .on_parse = [&call_count](const std::vector<std::string>&) -> void {
        ++call_count;
      },
  }};

  auto args_vec = argv({"prog", "--file", "a", "--file", "b", "--file", "c"});
  auto result = cli::Parser<CallbackListArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_EQ(call_count, 3);
}
