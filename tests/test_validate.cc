#include <gtest/gtest.h>

#include "argon/arithmetic_argument.hh"
#include "argon/bool_argument.hh"
#include "argon/error.hh"
#include "argon/parser.hh"
#include "argon/string_argument.hh"
#include "argon/validate.hh"

using namespace argon;

// ---- Built-in validators: argon::validate::positive ----

TEST(ValidatePositive, AcceptsPositiveInt) {
  EXPECT_TRUE(validate::positive(1));
  EXPECT_TRUE(validate::positive(42));
}

TEST(ValidatePositive, RejectsZero) {
  EXPECT_FALSE(validate::positive(0));
}

TEST(ValidatePositive, RejectsNegative) {
  EXPECT_FALSE(validate::positive(-1));
}

TEST(ValidatePositive, AcceptsPositiveDouble) {
  EXPECT_TRUE(validate::positive(0.1));
}

TEST(ValidatePositive, RejectsZeroDouble) {
  EXPECT_FALSE(validate::positive(0.0));
}

// ---- Built-in validators: argon::validate::non_negative ----

TEST(ValidateNonNegative, AcceptsZero) {
  EXPECT_TRUE(validate::non_negative(0));
}

TEST(ValidateNonNegative, AcceptsPositive) {
  EXPECT_TRUE(validate::non_negative(5));
}

TEST(ValidateNonNegative, RejectsNegative) {
  EXPECT_FALSE(validate::non_negative(-1));
}

// ---- Built-in validators: argon::validate::range ----

TEST(ValidateRange, AcceptsValueInRange) {
  EXPECT_TRUE(validate::range<1, 65535>(1));
  EXPECT_TRUE(validate::range<1, 65535>(8080));
  EXPECT_TRUE(validate::range<1, 65535>(65535));
}

TEST(ValidateRange, RejectsBelowMin) {
  EXPECT_FALSE(validate::range<1, 65535>(0));
}

TEST(ValidateRange, RejectsAboveMax) {
  EXPECT_FALSE(validate::range<1, 65535>(65536));
}

TEST(ValidateRange, ErrorMessageContainsBounds) {
  auto r = validate::range<1, 65535>(0);
  ASSERT_FALSE(r);
  EXPECT_NE(r.error().find("1"), std::string::npos);
  EXPECT_NE(r.error().find("65535"), std::string::npos);
}

// ---- Built-in validators: argon::validate::non_empty ----

TEST(ValidateNonEmpty, AcceptsNonEmptyString) {
  EXPECT_TRUE(validate::non_empty("hello"));
}

TEST(ValidateNonEmpty, RejectsEmptyString) {
  EXPECT_FALSE(validate::non_empty(""));
}

// ---- Built-in validators: argon::validate::one_of ----

TEST(ValidateOneOf, AcceptsValidChoice) {
  auto v = validate::one_of<std::string>({"debug", "info", "warn"});
  EXPECT_TRUE(v("debug"));
  EXPECT_TRUE(v("info"));
  EXPECT_TRUE(v("warn"));
}

TEST(ValidateOneOf, RejectsInvalidChoice) {
  auto v = validate::one_of<std::string>({"debug", "info", "warn"});
  EXPECT_FALSE(v("error"));
}

TEST(ValidateOneOf, ErrorMessageListsChoices) {
  auto v = validate::one_of<std::string>({"a", "b", "c"});
  auto r = v("x");
  ASSERT_FALSE(r);
  EXPECT_NE(r.error().find("a"), std::string::npos);
  EXPECT_NE(r.error().find("b"), std::string::npos);
  EXPECT_NE(r.error().find("c"), std::string::npos);
}

TEST(ValidateOneOf, WorksWithInts) {
  auto v = validate::one_of<int>({1, 2, 3});
  EXPECT_TRUE(v(2));
  EXPECT_FALSE(v(5));
}

// ---- Validator integrated with Arg: validation_failed error ----

struct PortArgs {
  IntArg<"port", 'p'> port{{
      .validator = validate::range<1, 65535>,
      .description = "Port number",
  }};
};

TEST(ValidatorIntegration, AcceptsValidPort) {
  Parser<PortArgs> parser;
  auto r = parser.parse({"--port", "8080"});
  ASSERT_TRUE(r) << r.error().what();
  EXPECT_EQ(r->port.value(), 8080);
}

TEST(ValidatorIntegration, RejectsPortZero) {
  Parser<PortArgs> parser;
  auto r = parser.parse({"--port", "0"});
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().code, ErrorCode::validation_failed);
}

TEST(ValidatorIntegration, RejectsPortTooLarge) {
  Parser<PortArgs> parser;
  auto r = parser.parse({"--port", "65536"});
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().code, ErrorCode::validation_failed);
}

TEST(ValidatorIntegration, ValidationErrorDetailContainsMessage) {
  Parser<PortArgs> parser;
  auto r = parser.parse({"--port", "0"});
  ASSERT_FALSE(r);
  EXPECT_FALSE(r.error().detail.empty());
}

// ---- Validator not called when argument not provided ----

struct OptionalWithValidator {
  IntArg<"count"> count{{
      .validator = validate::positive<int>,
  }};
};

TEST(ValidatorIntegration, NotCalledWhenNotProvided) {
  Parser<OptionalWithValidator> parser;
  // count not provided, validator should not fire (value stays 0 which is invalid)
  auto r = parser.parse({});
  EXPECT_TRUE(r) << r.error().what();
}

// ---- Custom lambda validator ----

struct EvenArgs {
  IntArg<"n"> n{{
      .validator = [](const int& v) -> std::expected<void, std::string> {
        if (v % 2 == 0) return {};
        return std::unexpected("must be even");
      },
  }};
};

TEST(ValidatorIntegration, CustomLambdaAcceptsEven) {
  Parser<EvenArgs> parser;
  auto r = parser.parse({"--n", "4"});
  ASSERT_TRUE(r) << r.error().what();
  EXPECT_EQ(r->n.value(), 4);
}

TEST(ValidatorIntegration, CustomLambdaRejectsOdd) {
  Parser<EvenArgs> parser;
  auto r = parser.parse({"--n", "3"});
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().code, ErrorCode::validation_failed);
  EXPECT_EQ(r.error().detail, "must be even");
}

// ---- Validator for string argument ----

struct LevelArgs {
  StrArg<"level"> level{{
      .validator = validate::one_of<std::string>({"debug", "info", "warn", "error"}),
      .description = "Log level",
  }};
};

TEST(ValidatorIntegration, StringOneOfAcceptsValid) {
  Parser<LevelArgs> parser;
  auto r = parser.parse({"--level", "info"});
  ASSERT_TRUE(r) << r.error().what();
  EXPECT_EQ(r->level.value(), "info");
}

TEST(ValidatorIntegration, StringOneOfRejectsInvalid) {
  Parser<LevelArgs> parser;
  auto r = parser.parse({"--level", "trace"});
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().code, ErrorCode::validation_failed);
}

// ---- Validator for positional argument ----

struct PosArgs {
  IntPositional n{Param<int>{
      .requirement = required,
      .validator = validate::positive<int>,
      .description = "Positive number",
  }};
};

TEST(ValidatorIntegration, PositionalAcceptsValid) {
  Parser<PosArgs> parser;
  auto r = parser.parse({"5"});
  ASSERT_TRUE(r) << r.error().what();
  EXPECT_EQ(r->n.value(), 5);
}

TEST(ValidatorIntegration, PositionalRejectsInvalid) {
  Parser<PosArgs> parser;
  auto r = parser.parse({"-1"});
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().code, ErrorCode::validation_failed);
}

// ---- Required arg with validator: missing arg error takes priority ----

struct RequiredPort {
  IntArg<"port"> port{{
      .requirement = required,
      .validator = validate::range<1, 65535>,
  }};
};

TEST(ValidatorIntegration, RequiredArgMissingGivesMissingError) {
  Parser<RequiredPort> parser;
  auto r = parser.parse({});
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().code, ErrorCode::missing_argument);
}
