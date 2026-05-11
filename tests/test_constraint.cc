#include <gtest/gtest.h>

#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "cli/constraint.hh"
#include "cli/parser.hh"

namespace {

auto argv(std::initializer_list<std::string_view> v)
    -> std::vector<std::string_view> {
  return {v};
}

// ── one_of ────────────────────────────────────────────────────────────────

struct OneOfArgs {
  cli::Flag<"foo", 'f'> foo{{.help = "", .presence = cli::optional}};
  cli::Flag<"bar", 'b'> bar{{.help = "", .presence = cli::optional}};
  cli::Flag<"baz", 'z'> baz{{.help = "", .presence = cli::optional}};

  auto constraints() -> cli::ConstraintResult {
    return cli::constraint::one_of(foo, bar, baz);
  }
};

TEST(Constraint, OneOfPassesWhenExactlyOneProvided) {
  auto args = argv({"prog", "--foo"});
  auto r =
      cli::Parser<OneOfArgs>{}.parse(std::span<const std::string_view>(args), 1);
  EXPECT_TRUE(r.has_value());
}

TEST(Constraint, OneOfFailsWhenNoneProvided) {
  auto args = argv({"prog"});
  auto r =
      cli::Parser<OneOfArgs>{}.parse(std::span<const std::string_view>(args), 1);
  ASSERT_TRUE(r.has_error());
  EXPECT_EQ(r.error.code, cli::ErrorCode::missing_required);
}

TEST(Constraint, OneOfFailsWhenMultipleProvided) {
  auto args = argv({"prog", "--foo", "--bar"});
  auto r =
      cli::Parser<OneOfArgs>{}.parse(std::span<const std::string_view>(args), 1);
  ASSERT_TRUE(r.has_error());
  EXPECT_EQ(r.error.code, cli::ErrorCode::mutually_exclusive);
}

// ── at_most_one_of ────────────────────────────────────────────────────────

struct AtMostOneArgs {
  cli::Flag<"verbose", 'v'> verbose{{.help = "", .presence = cli::optional}};
  cli::Flag<"quiet", 'q'> quiet{{.help = "", .presence = cli::optional}};

  auto constraints() -> cli::ConstraintResult {
    return cli::constraint::at_most_one_of(verbose, quiet);
  }
};

TEST(Constraint, AtMostOnePassesWhenNoneProvided) {
  auto args = argv({"prog"});
  auto r = cli::Parser<AtMostOneArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  EXPECT_TRUE(r.has_value());
}

TEST(Constraint, AtMostOnePassesWhenOneProvided) {
  auto args = argv({"prog", "--verbose"});
  auto r = cli::Parser<AtMostOneArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  EXPECT_TRUE(r.has_value());
}

TEST(Constraint, AtMostOneFailsWhenBothProvided) {
  auto args = argv({"prog", "--verbose", "--quiet"});
  auto r = cli::Parser<AtMostOneArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  ASSERT_TRUE(r.has_error());
  EXPECT_EQ(r.error.code, cli::ErrorCode::mutually_exclusive);
}

// ── all_or_none ───────────────────────────────────────────────────────────

struct AllOrNoneArgs {
  cli::IntOption<"host", 'h'> host{{.help = "", .presence = cli::optional}};
  cli::IntOption<"port", 'p'> port{{.help = "", .presence = cli::optional}};

  auto constraints() -> cli::ConstraintResult {
    return cli::constraint::all_or_none(host, port);
  }
};

TEST(Constraint, AllOrNonePassesWhenNoneProvided) {
  auto args = argv({"prog"});
  auto r = cli::Parser<AllOrNoneArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  EXPECT_TRUE(r.has_value());
}

TEST(Constraint, AllOrNonePassesWhenBothProvided) {
  auto args = argv({"prog", "--host", "1", "--port", "8080"});
  auto r = cli::Parser<AllOrNoneArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  EXPECT_TRUE(r.has_value());
}

TEST(Constraint, AllOrNoneFailsWhenOnlyOneProvided) {
  auto args = argv({"prog", "--port", "8080"});
  auto r = cli::Parser<AllOrNoneArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  ASSERT_TRUE(r.has_error());
  EXPECT_EQ(r.error.code, cli::ErrorCode::dependency_missing);
}

// ── requires_if ───────────────────────────────────────────────────────────

struct RequiresIfArgs {
  cli::Flag<"output", 'o'> output{{.help = "", .presence = cli::optional}};
  cli::Flag<"format", 'f'> format{{.help = "", .presence = cli::optional}};

  auto constraints() -> cli::ConstraintResult {
    return cli::constraint::requires_if(output, format);
  }
};

TEST(Constraint, RequiresIfPassesWhenConditionAbsent) {
  auto args = argv({"prog"});
  auto r = cli::Parser<RequiresIfArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  EXPECT_TRUE(r.has_value());
}

TEST(Constraint, RequiresIfPassesWhenBothPresent) {
  auto args = argv({"prog", "--output", "--format"});
  auto r = cli::Parser<RequiresIfArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  EXPECT_TRUE(r.has_value());
}

TEST(Constraint, RequiresIfFailsWhenConditionPresentButDepAbsent) {
  auto args = argv({"prog", "--output"});
  auto r = cli::Parser<RequiresIfArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  ASSERT_TRUE(r.has_error());
  EXPECT_EQ(r.error.code, cli::ErrorCode::dependency_missing);
  EXPECT_EQ(r.error.subject, "--format");
}

// ── operator&& chaining ───────────────────────────────────────────────────

struct ChainedArgs {
  cli::Flag<"alpha", 'a'> alpha{{.help = "", .presence = cli::optional}};
  cli::Flag<"beta", 'b'> beta{{.help = "", .presence = cli::optional}};
  cli::Flag<"gamma", 'g'> gamma{{.help = "", .presence = cli::optional}};

  auto constraints() -> cli::ConstraintResult {
    return cli::constraint::at_most_one_of(alpha, beta) &&
           cli::constraint::requires_if(beta, gamma);
  }
};

TEST(Constraint, ChainedPassesWhenAllSatisfied) {
  auto args = argv({"prog", "--beta", "--gamma"});
  auto r = cli::Parser<ChainedArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  EXPECT_TRUE(r.has_value());
}

TEST(Constraint, ChainedFirstFailureShortCircuits) {
  auto args = argv({"prog", "--alpha", "--beta"});
  auto r = cli::Parser<ChainedArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  ASSERT_TRUE(r.has_error());
  EXPECT_EQ(r.error.code, cli::ErrorCode::mutually_exclusive);
}

TEST(Constraint, ChainedSecondConstraintCaughtWhenFirstPasses) {
  auto args = argv({"prog", "--beta"});
  auto r = cli::Parser<ChainedArgs>{}.parse(
      std::span<const std::string_view>(args), 1);
  ASSERT_TRUE(r.has_error());
  EXPECT_EQ(r.error.code, cli::ErrorCode::dependency_missing);
  EXPECT_EQ(r.error.subject, "--gamma");
}

// ── no constraints() method — parser is unaffected ────────────────────────

TEST(Constraint, NoConstraintsMethodIsIgnored) {
  struct PlainArgs {
    cli::Flag<"verbose", 'v'> verbose{{.help = "", .presence = cli::optional}};
  };
  auto args = argv({"prog", "--verbose"});
  auto r =
      cli::Parser<PlainArgs>{}.parse(std::span<const std::string_view>(args), 1);
  EXPECT_TRUE(r.has_value());
}

}  // namespace
