#include <gtest/gtest.h>

#include "argon/argument.hh"

// IsValidLongOpt is consteval, so wrap each case in a lambda to call at runtime.
template <argon::StringLiteral Name>
constexpr bool IsLong = argon::detail::IsValidLongOpt<Name>();

TEST(Argument, OptionNameValidation) {
  // ---- specified cases ----
  EXPECT_TRUE((IsLong<"kebab-case">));    // valid kebab-case
  EXPECT_FALSE((IsLong<"Kebab-case">));   // leading uppercase
  EXPECT_TRUE((IsLong<"kebab0-case">));   // digit inside segment
  EXPECT_TRUE((IsLong<"kebab1">));        // trailing digit
  EXPECT_FALSE((IsLong<"-kebab">));       // leading hyphen
  EXPECT_FALSE((IsLong<"kebab-">));       // trailing hyphen
  EXPECT_FALSE((IsLong<"0kebab">));       // leading digit
  EXPECT_FALSE((IsLong<"?kebab">));       // leading special char
  EXPECT_FALSE((IsLong<"0Kebab-">));      // leading digit + trailing hyphen

  // ---- edge cases ----
  EXPECT_FALSE((IsLong<"">));             // empty string
  EXPECT_TRUE((IsLong<"a">));             // single lowercase letter
  EXPECT_TRUE((IsLong<"a-b">));           // minimal valid kebab
  EXPECT_TRUE((IsLong<"a1b">));           // digit in the middle
  EXPECT_TRUE((IsLong<"a-1">));           // digit as last segment
  EXPECT_TRUE((IsLong<"a-1-b">));         // digit segment between letters
  EXPECT_FALSE((IsLong<"kebab--case">));  // consecutive hyphens
  EXPECT_FALSE((IsLong<"UPPER">));        // all uppercase
  EXPECT_FALSE((IsLong<"camelCase">));    // uppercase mid-word
  EXPECT_FALSE((IsLong<"kebab_case">));   // underscore instead of hyphen
  EXPECT_FALSE((IsLong<"kebab case">));   // space inside name
  EXPECT_FALSE((IsLong<"kebab.case">));   // dot inside name
}

TEST(Argument, ShortOptionNameValidation) {
  EXPECT_TRUE(argon::detail::IsShortOptionName('a'));
  EXPECT_TRUE(argon::detail::IsShortOptionName('z'));
  EXPECT_TRUE(argon::detail::IsShortOptionName('A'));
  EXPECT_TRUE(argon::detail::IsShortOptionName('Z'));
  EXPECT_TRUE(argon::detail::IsShortOptionName('n'));
  EXPECT_FALSE(argon::detail::IsShortOptionName('0'));
  EXPECT_FALSE(argon::detail::IsShortOptionName('9'));
  EXPECT_FALSE(argon::detail::IsShortOptionName('-'));
  EXPECT_FALSE(argon::detail::IsShortOptionName('_'));
  EXPECT_FALSE(argon::detail::IsShortOptionName('?'));
  EXPECT_FALSE(argon::detail::IsShortOptionName(' '));
  EXPECT_FALSE(argon::detail::IsShortOptionName('\0'));
}
