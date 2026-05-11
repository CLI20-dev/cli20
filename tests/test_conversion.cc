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
using cli::conversion::Negatable;

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
  TempPathGuard(const TempPathGuard&) = default;
  TempPathGuard(TempPathGuard&&) = delete;
  auto operator=(const TempPathGuard&) -> TempPathGuard& = default;
  auto operator=(TempPathGuard&&) -> TempPathGuard& = delete;

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
  auto result = cli::conversion::choice<int, parse_mode>.invoke(
      ctx(2), ActionResult<std::string_view>::ok("fast"));

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value, 1);
}

TEST(Conversion, ChoiceReportsInvalidChoice) {
  auto result = cli::conversion::choice<int, parse_mode>.invoke(
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

TEST(Conversion, NegatableNoPrefixReturnsNameAndTrue) {
  auto r = Negatable<"no-">{}(ctx(0), ActionResult<std::string_view>::ok("lto"));
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r.value.name, "lto");
  EXPECT_TRUE(r.value.enabled);
}

TEST(Conversion, NegatablePrefixedReturnsStrippedNameAndFalse) {
  auto r =
      Negatable<"no-">{}(ctx(0), ActionResult<std::string_view>::ok("no-lto"));
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r.value.name, "lto");
  EXPECT_FALSE(r.value.enabled);
}

TEST(Conversion, NegatablePrefixOnlyYieldsEmptyName) {
  auto r = Negatable<"no-">{}(ctx(0), ActionResult<std::string_view>::ok("no-"));
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r.value.name, "");
  EXPECT_FALSE(r.value.enabled);
}

TEST(Conversion, NegatablePrefixInMiddleIsNotStripped) {
  auto r =
      Negatable<"no-">{}(ctx(0), ActionResult<std::string_view>::ok("lno-to"));
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r.value.name, "lno-to");
  EXPECT_TRUE(r.value.enabled);
}

// ── SimpleAction / PackAction CRTP helpers ────────────────────────────────

namespace {

// Parses "R,G,B" into std::tuple<int,int,int>.
struct RGBConversion {
  template <class Input>
  static constexpr bool accepts_input =
      cli::deduce_accepts_input<RGBConversion, Input>;

  template <class Input>
  using after_type = cli::deduce_after_type<RGBConversion, Input>;

  template <class Input>
  using storage_type = void;

  auto operator()(cli::ActionCtx<void> ctx,
                  cli::ActionResult<std::string_view> input) const
      -> cli::ActionResult<std::tuple<int, int, int>> {
    using Ret = cli::ActionResult<std::tuple<int, int, int>>;
    auto sv = input.value;
    auto p1 = sv.find(',');
    auto p2 = (p1 != sv.npos) ? sv.find(',', p1 + 1) : sv.npos;
    if (p1 == sv.npos || p2 == sv.npos) {
      return Ret::fail(
          cli::detail::invalid_value_error(cli::ErrorKind::conversion, ctx.index,
                                           std::string(sv), "expected R,G,B"));
    }
    auto to_int = [&](std::string_view s) -> std::optional<int> {
      int v = 0;
      auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
      if (ec != std::errc{} || ptr != s.data() + s.size()) return std::nullopt;
      return v;
    };
    auto r = to_int(sv.substr(0, p1));
    auto g = to_int(sv.substr(p1 + 1, p2 - p1 - 1));
    auto b = to_int(sv.substr(p2 + 1));
    if (!r || !g || !b) {
      return Ret::fail(cli::detail::invalid_value_error(
          cli::ErrorKind::conversion, ctx.index, std::string(sv),
          "each component must be an integer"));
    }
    return Ret::ok(std::make_tuple(*r, *g, *b));
  }
};

// Stores only the maximum value seen across multiple invocations.
struct StoreMax {
  template <class Prev>
  static constexpr bool accepts_input =
      !std::same_as<std::remove_cvref_t<Prev>, void>;

  template <class Prev>
  using after_type = void;

  template <class Prev>
  using storage_type = std::optional<std::remove_cvref_t<Prev>>;

  template <class Prev>
  auto operator()(cli::ActionCtx<storage_type<Prev>> ctx,
                  cli::ActionResult<Prev> input) const
      -> cli::ActionResult<void> {
    auto& stored = ctx.arg.get();
    if (!stored.has_value() || input.value > *stored) {
      stored = input.value;
    }
    return cli::ActionResult<void>::ok();
  }
};

// Compile-time trait assertions.
static_assert(RGBConversion::accepts_input<std::string_view>);
static_assert(!RGBConversion::accepts_input<int>);
static_assert(std::same_as<RGBConversion::after_type<std::string_view>,
                           std::tuple<int, int, int>>);
static_assert(std::same_as<RGBConversion::storage_type<std::string_view>, void>);

static_assert(StoreMax::accepts_input<int>);
static_assert(!StoreMax::accepts_input<void>);
static_assert(std::same_as<StoreMax::after_type<int>, void>);
static_assert(std::same_as<StoreMax::storage_type<int>, std::optional<int>>);

}  // namespace

TEST(SimpleAction, ConversionTraitsAndInvoke) {
  cli::ActionCtx<void> c;
  auto r =
      RGBConversion{}(c, cli::ActionResult<std::string_view>::ok("10,20,30"));
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r.value, (std::tuple{10, 20, 30}));
}

TEST(SimpleAction, ConversionBadInputFails) {
  cli::ActionCtx<void> c;
  auto r = RGBConversion{}(c, cli::ActionResult<std::string_view>::ok("bad"));
  EXPECT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, cli::ErrorCode::invalid_value);
}

TEST(PackAction, StoreMaxKeepsLargest) {
  std::optional<int> storage;
  cli::ActionCtx<std::optional<int>> c{.arg = std::ref(storage)};

  StoreMax{}(c, cli::ActionResult<int>::ok(5));
  StoreMax{}(c, cli::ActionResult<int>::ok(9));
  StoreMax{}(c, cli::ActionResult<int>::ok(3));

  ASSERT_TRUE(storage.has_value());
  EXPECT_EQ(*storage, 9);
}

TEST(SimpleAction, PipelineWithSimpleAndPackActions) {
  constexpr auto pipeline =
      cli::Action<RGBConversion{}>{} | cli::Action<StoreMax{}>{};

  std::optional<std::tuple<int, int, int>> storage;
  cli::ActionCtx<std::optional<std::tuple<int, int, int>>> ctx{
      .arg = std::ref(storage)};

  auto r1 =
      pipeline.invoke(ctx, cli::ActionResult<std::string_view>::ok("1,2,3"));
  ASSERT_TRUE(r1.has_value());
  auto r2 =
      pipeline.invoke(ctx, cli::ActionResult<std::string_view>::ok("5,6,7"));
  ASSERT_TRUE(r2.has_value());

  ASSERT_TRUE(storage.has_value());
  EXPECT_EQ(*storage, (std::tuple{5, 6, 7}));
}
