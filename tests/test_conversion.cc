#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <string_view>

#include "cli/action.hh"

namespace fs = std::filesystem;

namespace {

using cli::ActionCtx;
using cli::ActionResult;
using cli::ErrorCode;
using cli::conversion::Bool;
using cli::conversion::ExistingDirectory;
using cli::conversion::ExistingFile;
using cli::conversion::Floating;
using cli::conversion::Integer;

constexpr auto parse_mode = [](std::string_view value) -> std::optional<int> {
  if (value == "fast") {
    return 1;
  }
  if (value == "slow") {
    return 2;
  }
  return std::nullopt;
};

class TempPathGuard {
 public:
  explicit TempPathGuard(fs::path path) : path_(std::move(path)) {}

  ~TempPathGuard() {
    std::error_code ec;
    fs::remove_all(path_, ec);
  }

  [[nodiscard]] auto path() const -> const fs::path& { return path_; }

 private:
  fs::path path_;
};

auto ctx(std::size_t index) -> ActionCtx<void> {
  ActionCtx<void> out;
  out.index = index;
  return out;
}

}  // namespace

static_assert(std::same_as<Bool::after_type<std::string_view>, bool>);

TEST(Conversion, IntegerRejectsTrailingCharacters) {
  auto result =
      Integer<int>{}(ctx(3), ActionResult<std::string_view>::ok("42ms"));

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, ErrorCode::invalid_value);
  EXPECT_EQ(result.error.detail, "unexpected trailing characters");
  EXPECT_EQ(result.error.position, 3);
}

TEST(Conversion, FloatingParsesValue) {
  auto result =
      Floating<double>{}(ctx(1), ActionResult<std::string_view>::ok("3.25"));

  ASSERT_TRUE(result.has_value());
  EXPECT_DOUBLE_EQ(result.value, 3.25);
}

TEST(Conversion, FloatingRejectsTrailingCharacters) {
  auto result =
      Floating<float>{}(ctx(4), ActionResult<std::string_view>::ok("1.5f"));

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, ErrorCode::invalid_value);
  EXPECT_EQ(result.error.detail, "unexpected trailing characters");
}

TEST(Conversion, ChoiceUsesMapper) {
  auto result = cli::conversion::choice<int, parse_mode>(
      ctx(2), ActionResult<std::string_view>::ok("fast"));

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value, 1);
}

TEST(Conversion, ChoiceReportsInvalidChoice) {
  auto result = cli::conversion::choice<int, parse_mode>(
      ctx(2), ActionResult<std::string_view>::ok("medium"));

  ASSERT_TRUE(result.has_error());
  EXPECT_EQ(result.error.code, ErrorCode::invalid_choice);
}

TEST(Conversion, ExistingFileRequiresRegularFile) {
  auto temp_root =
      fs::temp_directory_path() / "cli20_test_conversion_existing_file";
  fs::create_directories(temp_root);
  TempPathGuard guard(temp_root);

  const auto file_path = temp_root / "input.txt";
  std::ofstream(file_path) << "content";

  auto ok = ExistingFile{}(
      ctx(5), ActionResult<std::string_view>::ok(file_path.string()));
  ASSERT_TRUE(ok.has_value());
  EXPECT_EQ(ok.value, file_path);

  auto fail = ExistingFile{}(
      ctx(6), ActionResult<std::string_view>::ok(temp_root.string()));
  ASSERT_TRUE(fail.has_error());
  EXPECT_EQ(fail.error.code, ErrorCode::invalid_value);
}

TEST(Conversion, ExistingDirectoryRequiresDirectory) {
  auto temp_root =
      fs::temp_directory_path() / "cli20_test_conversion_existing_directory";
  fs::create_directories(temp_root);
  TempPathGuard guard(temp_root);

  auto ok = ExistingDirectory{}(
      ctx(7), ActionResult<std::string_view>::ok(temp_root.string()));
  ASSERT_TRUE(ok.has_value());
  EXPECT_EQ(ok.value, temp_root);
}
