#include <gtest/gtest.h>

#include "cli/parser.hh"

// ---- Test infrastructure --------------------------------------------------

using cli::ErrorCode;
using cli::Nargs;
using cli::Token;
using cli::TokenizeResult;
using cli::TokenType;

using SpecMap = std::unordered_map<std::string, Nargs>;
using CmdSet = std::unordered_set<std::string>;

// Nargs shorthands
static constexpr Nargs kFlag = {.min = 0, .max = 0};       // no values
static constexpr Nargs kOne = {.min = 1, .max = 1};        // exactly 1
static constexpr Nargs kTwo = {.min = 2, .max = 2};        // exactly 2
static constexpr Nargs kOnePlus = {.min = 1, .max = -1};   // one or more
static constexpr Nargs kZeroPlus = {.min = 0, .max = -1};  // zero or more

// Wrap initializer_list so callers can write tok({"a", "b"}).
static auto tok(std::initializer_list<std::string_view> il) {
  return std::vector<std::string_view>(il);
}

// Invoke tokenize with vector args.
static auto tokenize(std::initializer_list<std::string_view> args,
                     const SpecMap& spec, const CmdSet& cmds = {})
    -> TokenizeResult {
  return cli::tokenize(tok(args), spec, cmds);
}

// Collect texts of tokens with the given type, in order.
static auto texts(const TokenizeResult& r, TokenType t)
    -> std::vector<std::string_view> {
  std::vector<std::string_view> out;
  for (const auto& tk : r.tokens) {
    if (tk.type == t) out.push_back(tk.text);
  }
  return out;
}

// Collect all tokens of a given type.
static auto of_type(const TokenizeResult& r, TokenType t) -> std::vector<Token> {
  std::vector<Token> out;
  for (const auto& tk : r.tokens) {
    if (tk.type == t) out.push_back(tk);
  }
  return out;
}

// ---- success: empty / positional ------------------------------------------

TEST(Tokenize, EmptyArgs) {
  auto r = tokenize({}, {});
  ASSERT_TRUE(r.has_value());
  EXPECT_TRUE(r.tokens.empty());
}

TEST(Tokenize, SinglePositional) {
  auto r = tokenize({"hello"}, {});
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r.tokens.size(), 1u);
  EXPECT_EQ(r.tokens[0].type, TokenType::positional);
  EXPECT_EQ(r.tokens[0].text, "hello");
  EXPECT_EQ(r.tokens[0].position, 0u);
}

TEST(Tokenize, MultiplePositionals) {
  auto r = tokenize({"a", "b", "c"}, {});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::positional),
            (std::vector<std::string_view>{"a", "b", "c"}));
  EXPECT_TRUE(of_type(r, TokenType::option).empty());
}

// ---- success: flags -------------------------------------------------------

TEST(Tokenize, FlagEmitsOptionTokenOnly) {
  auto r = tokenize({"--verbose"}, {{"--verbose", kFlag}});
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r.tokens.size(), 1u);
  EXPECT_EQ(r.tokens[0].type, TokenType::option);
  EXPECT_EQ(r.tokens[0].text, "verbose");
  EXPECT_EQ(r.tokens[0].matched_prefix, "--");
}

TEST(Tokenize, FlagDoesNotConsumeNextToken) {
  auto r = tokenize({"--verbose", "file.txt"}, {{"--verbose", kFlag}});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::option),
            (std::vector<std::string_view>{"verbose"}));
  EXPECT_EQ(texts(r, TokenType::positional),
            (std::vector<std::string_view>{"file.txt"}));
  EXPECT_TRUE(of_type(r, TokenType::value).empty());
}

TEST(Tokenize, TwoFlagsInSequence) {
  auto r = tokenize({"--foo", "--bar"}, {{"--foo", kFlag}, {"--bar", kFlag}});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::option),
            (std::vector<std::string_view>{"foo", "bar"}));
  EXPECT_TRUE(of_type(r, TokenType::value).empty());
}

// ---- success: option + values ---------------------------------------------

TEST(Tokenize, OptionExactlyOneValue) {
  auto r = tokenize({"--name", "Alice"}, {{"--name", kOne}});
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r.tokens.size(), 2u);
  EXPECT_EQ(r.tokens[0].type, TokenType::option);
  EXPECT_EQ(r.tokens[0].text, "name");
  EXPECT_EQ(r.tokens[0].matched_prefix, "--");
  EXPECT_EQ(r.tokens[0].position, 0u);
  EXPECT_EQ(r.tokens[1].type, TokenType::value);
  EXPECT_EQ(r.tokens[1].text, "Alice");
  EXPECT_EQ(r.tokens[1].position, 1u);
}

TEST(Tokenize, OptionExactlyTwoValues) {
  auto r = tokenize({"--range", "1", "2"}, {{"--range", kTwo}});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::value),
            (std::vector<std::string_view>{"1", "2"}));
}

TEST(Tokenize, OptionStopsAtNextKnownOption) {
  auto r = tokenize({"--name", "Alice", "--out", "f.txt"},
                    {{"--name", kOnePlus}, {"--out", kOne}});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::value),
            (std::vector<std::string_view>{"Alice", "f.txt"}));
  // option tokens in order
  EXPECT_EQ(texts(r, TokenType::option),
            (std::vector<std::string_view>{"name", "out"}));
}

TEST(Tokenize, ZeroOrMoreConsumesAllBareValues) {
  auto r = tokenize({"--list", "a", "b", "c"}, {{"--list", kZeroPlus}});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::value),
            (std::vector<std::string_view>{"a", "b", "c"}));
}

TEST(Tokenize, ZeroOrMoreWithNoValuesAtEndOfArgs) {
  auto r = tokenize({"--list"}, {{"--list", kZeroPlus}});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(of_type(r, TokenType::option).size(), 1u);
  EXPECT_TRUE(of_type(r, TokenType::value).empty());
}

TEST(Tokenize, ZeroOrMoreStopsBeforeNextKnownOption) {
  auto r = tokenize({"--list", "a", "--flag"},
                    {{"--list", kZeroPlus}, {"--flag", kFlag}});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::value), (std::vector<std::string_view>{"a"}));
  EXPECT_EQ(texts(r, TokenType::option),
            (std::vector<std::string_view>{"list", "flag"}));
}

TEST(Tokenize, OnePlusCollectsMultipleValues) {
  auto r = tokenize({"--files", "a.txt", "b.txt"}, {{"--files", kOnePlus}});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::value),
            (std::vector<std::string_view>{"a.txt", "b.txt"}));
}

// ---- success: --opt=val inline syntax ------------------------------------

TEST(Tokenize, InlineSyntaxSplitsIntoOptionAndValueAtSamePosition) {
  auto r = tokenize({"--name=Alice"}, {{"--name", kOne}});
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r.tokens.size(), 2u);
  EXPECT_EQ(r.tokens[0].type, TokenType::option);
  EXPECT_EQ(r.tokens[0].text, "name");
  EXPECT_EQ(r.tokens[0].matched_prefix, "--");
  EXPECT_EQ(r.tokens[0].position, 0u);
  EXPECT_EQ(r.tokens[1].type, TokenType::value);
  EXPECT_EQ(r.tokens[1].text, "Alice");
  EXPECT_EQ(r.tokens[1].position, 0u);  // same position as option
}

TEST(Tokenize, InlineSyntaxEmptyValue) {
  auto r = tokenize({"--tag="}, {{"--tag", kOne}});
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r.tokens.size(), 2u);
  EXPECT_EQ(r.tokens[1].text, "");
}

TEST(Tokenize, InlineSyntaxValueContainsEquals) {
  // Only the first '=' is the separator; rest is the value.
  auto r = tokenize({"--expr=a=b"}, {{"--expr", kOne}});
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r.tokens.size(), 2u);
  EXPECT_EQ(r.tokens[1].text, "a=b");
}

TEST(Tokenize, InlineSyntaxDoesNotConsumeFollowingArgs) {
  // --name=Alice Bob  →  value["Alice"], positional["Bob"]
  auto r = tokenize({"--name=Alice", "Bob"}, {{"--name", kOnePlus}});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::value),
            (std::vector<std::string_view>{"Alice"}));
  EXPECT_EQ(texts(r, TokenType::positional),
            (std::vector<std::string_view>{"Bob"}));
}

// ---- success: mixed positionals and options -------------------------------

TEST(Tokenize, PositionalBeforeAndAfterOption) {
  auto r = tokenize({"in.txt", "--out", "out.txt", "extra"}, {{"--out", kOne}});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::positional),
            (std::vector<std::string_view>{"in.txt", "extra"}));
  EXPECT_EQ(texts(r, TokenType::value),
            (std::vector<std::string_view>{"out.txt"}));
}

TEST(Tokenize, TokenOrderIsPreserved) {
  // Verify the full token sequence is in input order.
  auto r = tokenize({"pos1", "--name", "Alice", "--verbose", "pos2"},
                    {{"--name", kOne}, {"--verbose", kFlag}});
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r.tokens.size(), 5u);
  EXPECT_EQ(r.tokens[0].type, TokenType::positional);
  EXPECT_EQ(r.tokens[0].text, "pos1");
  EXPECT_EQ(r.tokens[1].type, TokenType::option);
  EXPECT_EQ(r.tokens[1].text, "name");
  EXPECT_EQ(r.tokens[2].type, TokenType::value);
  EXPECT_EQ(r.tokens[2].text, "Alice");
  EXPECT_EQ(r.tokens[3].type, TokenType::option);
  EXPECT_EQ(r.tokens[3].text, "verbose");
  EXPECT_EQ(r.tokens[4].type, TokenType::positional);
  EXPECT_EQ(r.tokens[4].text, "pos2");
}

// ---- success: "--" separator ----------------------------------------------

TEST(Tokenize, SeparatorMakesEverythingAfterPositional) {
  auto r =
      tokenize({"--name", "Alice", "--", "--foo", "bar"}, {{"--name", kOne}});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::value),
            (std::vector<std::string_view>{"Alice"}));
  EXPECT_EQ(texts(r, TokenType::positional),
            (std::vector<std::string_view>{"--foo", "bar"}));
}

TEST(Tokenize, SeparatorAlone) {
  auto r = tokenize({"--", "a", "b"}, {});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::positional),
            (std::vector<std::string_view>{"a", "b"}));
}

TEST(Tokenize, SeparatorAtEnd) {
  auto r = tokenize({"--flag", "--"}, {{"--flag", kFlag}});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(of_type(r, TokenType::option).size(), 1u);
  EXPECT_TRUE(of_type(r, TokenType::positional).empty());
}

TEST(Tokenize, SeparatorStopsValueConsumption) {
  // --name Alice -- extra  →  value["Alice"], positional["extra"]
  auto r = tokenize({"--name", "Alice", "--", "extra"}, {{"--name", kOnePlus}});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::value),
            (std::vector<std::string_view>{"Alice"}));
  EXPECT_EQ(texts(r, TokenType::positional),
            (std::vector<std::string_view>{"extra"}));
}

// ---- success: command -----------------------------------------------------

TEST(Tokenize, CommandEmitsCommandTokenAndStops) {
  auto r = tokenize({"run"}, {}, {"run"});
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r.tokens.size(), 1u);
  EXPECT_EQ(r.tokens[0].type, TokenType::command);
  EXPECT_EQ(r.tokens[0].text, "run");
  EXPECT_EQ(r.tokens[0].position, 0u);
}

TEST(Tokenize, CommandTailNotParsed) {
  // Everything after the command name is not in tokens.
  auto r = tokenize({"run", "--foo", "bar"}, {}, {"run"});
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r.tokens.size(), 1u);
  EXPECT_EQ(r.tokens[0].type, TokenType::command);
}

TEST(Tokenize, OptionsBeforeCommandAreParsed) {
  auto r = tokenize({"--cfg", "c.toml", "run", "--x", "1"}, {{"--cfg", kOne}},
                    {"run"});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::value),
            (std::vector<std::string_view>{"c.toml"}));
  auto cmds = of_type(r, TokenType::command);
  ASSERT_EQ(cmds.size(), 1u);
  EXPECT_EQ(cmds[0].text, "run");
  EXPECT_EQ(cmds[0].position, 2u);
}

TEST(Tokenize, CommandPositionIsCorrect) {
  // args[0]="--flag", args[1]="run" → command token at position 1
  auto r = tokenize({"--flag", "run"}, {{"--flag", kFlag}}, {"run"});
  ASSERT_TRUE(r.has_value());
  auto cmds = of_type(r, TokenType::command);
  ASSERT_EQ(cmds.size(), 1u);
  EXPECT_EQ(cmds[0].position, 1u);
}

TEST(Tokenize, CommandStopsValueConsumption) {
  // --list a run --x  →  value["a"], command["run"]
  auto r =
      tokenize({"--list", "a", "run", "--x"}, {{"--list", kZeroPlus}}, {"run"});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::value), (std::vector<std::string_view>{"a"}));
  EXPECT_EQ(of_type(r, TokenType::command).size(), 1u);
}

// ---- success: unknown-option-looking tokens as values ---------------------

// CLI11-compatible behaviour: an unknown "--xxx" token may be consumed as the
// first and only value when the option still needs at least one value.

TEST(Tokenize, UnknownOptionLookingConsumedAsFirstValue) {
  // --name --bar  →  value["--bar"]  (min=1, count=0 → consume)
  auto r = tokenize({"--name", "--bar"}, {{"--name", kOne}});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::value),
            (std::vector<std::string_view>{"--bar"}));
}

TEST(Tokenize, UnknownOptionLookingStopsAfterFirstRealValue) {
  // --name ba --baz  →  value["ba"], then --baz is top-level unknown → error
  auto r = tokenize({"--name", "ba", "--baz"}, {{"--name", kOnePlus}});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::unknown_option);
  EXPECT_EQ(r.error.subject, "--baz");
}

TEST(Tokenize, ZeroOrMoreStopsAtUnknownOptionLooking) {
  // min=0 → do not consume unknown option-looking even as first value
  auto r = tokenize({"--list", "--unknown"}, {{"--list", kZeroPlus}});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::unknown_option);
  EXPECT_EQ(r.error.subject, "--unknown");
}

// ---- error: unknown option ------------------------------------------------

TEST(Tokenize, ErrorUnknownOptionLong) {
  auto r = tokenize({"--unknown"}, {});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::unknown_option);
  EXPECT_EQ(r.error.subject, "--unknown");
  EXPECT_EQ(r.error.position, 0);
}

TEST(Tokenize, ErrorUnknownOptionAtNonZeroPosition) {
  auto r = tokenize({"pos", "--unknown"}, {});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::unknown_option);
  EXPECT_EQ(r.error.position, 1);
}

TEST(Tokenize, ErrorUnknownOptionWithInlineSyntax) {
  auto r = tokenize({"--unknown=val"}, {});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::unknown_option);
  EXPECT_EQ(r.error.subject, "--unknown");
}

// ---- error: missing value -------------------------------------------------

TEST(Tokenize, ErrorMissingValueAtEndOfArgs) {
  auto r = tokenize({"--name"}, {{"--name", kOne}});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::missing_value);
  EXPECT_EQ(r.error.subject, "--name");
}

TEST(Tokenize, ErrorMissingValueStopsAtNextKnownOption) {
  auto r = tokenize({"--name", "--flag"}, {{"--name", kOne}, {"--flag", kFlag}});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::missing_value);
  EXPECT_EQ(r.error.subject, "--name");
}

TEST(Tokenize, ErrorMissingValueExactTwoGotOne) {
  auto r = tokenize({"--range", "1"}, {{"--range", kTwo}});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::missing_value);
  EXPECT_EQ(r.error.subject, "--range");
}

TEST(Tokenize, ErrorMissingValueExactTwoGotZero) {
  auto r = tokenize({"--range"}, {{"--range", kTwo}});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::missing_value);
  EXPECT_EQ(r.error.subject, "--range");
}

TEST(Tokenize, ErrorMissingValueOnePlusGotZero) {
  auto r = tokenize({"--list"}, {{"--list", kOnePlus}});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::missing_value);
  EXPECT_EQ(r.error.subject, "--list");
}

TEST(Tokenize, ErrorMissingValueBeforeSeparator) {
  // --name -- extra  →  min=1 not satisfied before "--"
  auto r = tokenize({"--name", "--", "extra"}, {{"--name", kOne}});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::missing_value);
  EXPECT_EQ(r.error.subject, "--name");
}

// ---- error: flag with inline value ----------------------------------------

TEST(Tokenize, ErrorFlagWithInlineValue) {
  auto r = tokenize({"--verbose=true"}, {{"--verbose", kFlag}});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::invalid_value);
  EXPECT_EQ(r.error.subject, "--verbose");
}

// ---- error: --opt=val when min > 1 ----------------------------------------

TEST(Tokenize, ErrorInlineSyntaxCannotSatisfyMinTwo) {
  auto r = tokenize({"--range=1"}, {{"--range", kTwo}});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::missing_value);
  EXPECT_EQ(r.error.subject, "--range");
}

// ---- multi-prefix (TokenizerConfig) ---------------------------------------

static auto tokenize_cfg(std::initializer_list<std::string_view> args,
                         const SpecMap& spec, const cli::TokenizerConfig& cfg,
                         const CmdSet& cmds = {}) -> TokenizeResult {
  return cli::tokenize(tok(args), spec, cmds, cfg);
}

TEST(TokenizeMultiPrefix, AltPrefixRecognised) {
  // "+verbose" recognised as option when "+" is in option_prefixes
  cli::TokenizerConfig cfg{.option_prefixes = {"--", "+"}};
  auto r = tokenize_cfg({"+verbose"}, {{"+verbose", kFlag}}, cfg);
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r.tokens.size(), 1u);
  EXPECT_EQ(r.tokens[0].type, TokenType::option);
  EXPECT_EQ(r.tokens[0].text, "verbose");
  EXPECT_EQ(r.tokens[0].matched_prefix, "+");
}

TEST(TokenizeMultiPrefix, MatchedPrefixDistinguishesPrefixes) {
  // "--foo" and "+foo" are different options; matched_prefix tells them apart
  cli::TokenizerConfig cfg{.option_prefixes = {"--", "+"}};
  SpecMap spec{{"--foo", kFlag}, {"+foo", kFlag}};
  auto r = tokenize_cfg({"--foo", "+foo"}, spec, cfg);
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r.tokens.size(), 2u);
  EXPECT_EQ(r.tokens[0].matched_prefix, "--");
  EXPECT_EQ(r.tokens[1].matched_prefix, "+");
}

TEST(TokenizeMultiPrefix, DefaultPrefixStillWorks) {
  // Default config ({"--"}) behaves identically to before
  auto r = tokenize_cfg({"--name", "Alice"}, {{"--name", kOne}}, {});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r.tokens[0].matched_prefix, "--");
  EXPECT_EQ(r.tokens[1].text, "Alice");
  EXPECT_TRUE(r.tokens[1].matched_prefix.empty());
}

TEST(TokenizeMultiPrefix, UnknownAltPrefixTokenIsError) {
  cli::TokenizerConfig cfg{.option_prefixes = {"--", "+"}};
  auto r = tokenize_cfg({"+unknown"}, {{"--foo", kFlag}}, cfg);
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::unknown_option);
  EXPECT_EQ(r.error.subject, "+unknown");
}

// ---- inline_value_separator -----------------------------------------------

TEST(TokenizeInlineSep, ColonSeparator) {
  cli::TokenizerConfig cfg{.inline_value_separator = ':'};
  auto r = tokenize_cfg({"--name:Alice"}, {{"--name", kOne}}, cfg);
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r.tokens.size(), 2u);
  EXPECT_EQ(r.tokens[0].text, "name");
  EXPECT_EQ(r.tokens[1].text, "Alice");
}

TEST(TokenizeInlineSep, DefaultEqualsStillWorks) {
  // Sanity: default '=' still splits correctly.
  auto r = tokenize_cfg({"--name=Alice"}, {{"--name", kOne}}, {});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r.tokens[1].text, "Alice");
}

TEST(TokenizeInlineSep, EqualsTreatedAsValueWhenSepIsColon) {
  // With sep=':', "--name=Alice" has no separator → entire token is option name.
  cli::TokenizerConfig cfg{.inline_value_separator = ':'};
  auto r = tokenize_cfg({"--name=Alice"}, {{"--name=Alice", kFlag}}, cfg);
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r.tokens[0].text, "name=Alice");
}

// ---- short cluster: -xvf -------------------------------------------------

TEST(TokenizeCluster, AllFlagsExpanded) {
  SpecMap spec{{"-x", kFlag}, {"-v", kFlag}, {"-f", kFlag}};
  auto r = tokenize({"-xvf"}, spec);
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::option),
            (std::vector<std::string_view>{"x", "v", "f"}));
  EXPECT_TRUE(of_type(r, TokenType::value).empty());
  // All three share the same original arg position
  for (const auto& t : of_type(r, TokenType::option))
    EXPECT_EQ(t.position, 0u);
}

TEST(TokenizeCluster, SingleFlagStillWorks) {
  // A single known short flag should still be handled by the existing path.
  SpecMap spec{{"-v", kFlag}};
  auto r = tokenize({"-v"}, spec);
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r.tokens.size(), 1u);
  EXPECT_EQ(r.tokens[0].text, "v");
  EXPECT_EQ(r.tokens[0].matched_prefix, "-");
}

TEST(TokenizeCluster, FlagsThenValueNextToken) {
  // -xo val  →  flag 'x', option 'o', value "val"
  SpecMap spec{{"-x", kFlag}, {"-o", kOne}};
  auto r = tokenize({"-xo", "val"}, spec);
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::option),
            (std::vector<std::string_view>{"x", "o"}));
  EXPECT_EQ(texts(r, TokenType::value),
            (std::vector<std::string_view>{"val"}));
}

TEST(TokenizeCluster, AttachedValueOnly) {
  // -ofile  →  option 'o', value "file"
  SpecMap spec{{"-o", kOne}};
  auto r = tokenize({"-ofile"}, spec);
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r.tokens.size(), 2u);
  EXPECT_EQ(r.tokens[0].text, "o");
  EXPECT_EQ(r.tokens[0].matched_prefix, "-");
  EXPECT_EQ(r.tokens[1].type, TokenType::value);
  EXPECT_EQ(r.tokens[1].text, "file");
  EXPECT_EQ(r.tokens[1].position, 0u);
}

TEST(TokenizeCluster, FlagsThenAttachedValue) {
  // -xofile  →  flag 'x', option 'o', attached value "file"
  SpecMap spec{{"-x", kFlag}, {"-o", kOne}};
  auto r = tokenize({"-xofile"}, spec);
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(texts(r, TokenType::option),
            (std::vector<std::string_view>{"x", "o"}));
  EXPECT_EQ(texts(r, TokenType::value),
            (std::vector<std::string_view>{"file"}));
}

TEST(TokenizeCluster, UnknownCharInCluster) {
  SpecMap spec{{"-x", kFlag}};
  auto r = tokenize({"-xy"}, spec);
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::unknown_option);
  EXPECT_EQ(r.error.subject, "-y");
}

TEST(TokenizeCluster, MissingValueForLastChar) {
  // -xo with no following token → missing_value for 'o'
  SpecMap spec{{"-x", kFlag}, {"-o", kOne}};
  auto r = tokenize({"-xo"}, spec);
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error.code, ErrorCode::missing_value);
  EXPECT_EQ(r.error.subject, "-o");
}
