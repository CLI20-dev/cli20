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

struct BoundIntArgs {
  cli::BoundIntOption<"port", 'p'> port_opt;
};

struct BoundStringArgs {
  cli::BoundStringOption<"output", 'o'> output_opt;
};

struct BoundDoubleArgs {
  cli::BoundDoubleOption<"ratio"> ratio_opt;
};

struct MultiArgs {
  cli::BoundIntOption<"port", 'p'> port_opt;
  cli::BoundStringOption<"output", 'o'> output_opt;
};

}  // namespace

TEST(StoreInto, BoundIntOptionWritesValue) {
  int port = 0;
  BoundIntArgs args;
  args.port_opt = cli::BoundIntOption<"port", 'p'>{port};

  auto args_vec = argv({"prog", "--port", "8080"});
  auto result = cli::Parser<BoundIntArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_EQ(port, 8080);
}

TEST(StoreInto, BoundIntOptionShortName) {
  int port = 0;
  BoundIntArgs args;
  args.port_opt = cli::BoundIntOption<"port", 'p'>{port};

  auto args_vec = argv({"prog", "-p", "1234"});
  auto result = cli::Parser<BoundIntArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_EQ(port, 1234);
}

TEST(StoreInto, BoundStringOptionWritesValue) {
  std::string output;
  BoundStringArgs args;
  args.output_opt = cli::BoundStringOption<"output", 'o'>{output};

  auto args_vec = argv({"prog", "--output", "result.txt"});
  auto result = cli::Parser<BoundStringArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_EQ(output, "result.txt");
}

TEST(StoreInto, BoundDoubleOptionWritesValue) {
  double ratio = 0.0;
  BoundDoubleArgs args;
  args.ratio_opt = cli::BoundDoubleOption<"ratio">{ratio};

  auto args_vec = argv({"prog", "--ratio", "3.14"});
  auto result = cli::Parser<BoundDoubleArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_DOUBLE_EQ(ratio, 3.14);
}

TEST(StoreInto, NotProvidedLeavesVariableUnchanged) {
  int port = 9999;
  BoundIntArgs args;
  args.port_opt = cli::BoundIntOption<"port", 'p'>{port};

  auto args_vec = argv({"prog"});
  auto result = cli::Parser<BoundIntArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_EQ(port, 9999);  // unchanged
}

TEST(StoreInto, InvalidValueFails) {
  int port = 0;
  BoundIntArgs args;
  args.port_opt = cli::BoundIntOption<"port", 'p'>{port};

  auto args_vec = argv({"prog", "--port", "notanint"});
  auto result = cli::Parser<BoundIntArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::invalid_value);
  EXPECT_EQ(port, 0);  // unchanged on error
}

TEST(StoreInto, UnboundOptionFailsValidation) {
  BoundIntArgs args;

  auto args_vec = argv({"prog", "--port", "8080"});
  auto result = cli::Parser<BoundIntArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, cli::ErrorCode::validation_failed);
  EXPECT_EQ(result.error.kind, cli::ErrorKind::validation);
  EXPECT_EQ(result.error.subject, "store_into");
  EXPECT_EQ(result.error.detail,
            "target pointer is null; did you forget to call bind()?");
  EXPECT_EQ(result.error.position, 2);
}

TEST(StoreInto, MultipleVariablesBoundToSameArgs) {
  int port = 0;
  std::string output;
  MultiArgs args;
  args.port_opt = cli::BoundIntOption<"port", 'p'>{port};
  args.output_opt = cli::BoundStringOption<"output", 'o'>{output};

  auto args_vec = argv({"prog", "--port", "443", "--output", "out.bin"});
  auto result = cli::Parser<MultiArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_EQ(port, 443);
  EXPECT_EQ(output, "out.bin");
}

TEST(StoreInto, DirectArgImplWithStoreInto) {
  struct DirectArgs {
    cli::ArgImpl<"port", 'p', cli::nargs::one,
                 cli::conversion::integer<int> | cli::pack::store_into<int>>
        port_opt;
  };

  int port = 0;
  DirectArgs args;
  args.port_opt = decltype(args.port_opt){port};

  auto args_vec = argv({"prog", "--port", "5000"});
  auto result = cli::Parser<DirectArgs>{}.parse(
      std::move(args), std::span<const std::string_view>(args_vec), 1);

  ASSERT_TRUE(result.has_value()) << result.error.message();
  EXPECT_EQ(port, 5000);
}
