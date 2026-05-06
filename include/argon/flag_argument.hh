#pragma once

#include <argon/argument.hh>

namespace argon {

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::is_valid_long_option_name<LongOpt>() &&
           detail::is_valid_short_option_name(ShortOpt))
struct Flag : ArgumentTag {
  static constexpr auto type = ArgumentType::flag;

  constexpr Flag(std::string_view desc = {}) noexcept : description_(desc) {}

  [[nodiscard]] constexpr auto provided() const noexcept -> bool {
    return provided_;
  }
  [[nodiscard]] constexpr auto description() const noexcept -> std::string_view {
    return description_;
  }

 protected:
  [[nodiscard]] static constexpr auto longOpt() -> std::string_view {
    return LongOpt.view();
  }
  [[nodiscard]] static constexpr auto shortOpt() -> char { return ShortOpt; }

  template <class>
  friend class Parser;

  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }
  constexpr auto markProvided() noexcept -> void { provided_ = true; }

 private:
  detail::Nargs nargs_ = nargs::none;
  std::string_view description_;
  bool provided_ = false;
};

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() &&
           detail::IsValidShortOpt(ShortOpt))
using FlagArg = Flag<LongOpt, ShortOpt>;

// A flag that signals "show help".  When the parser detects this flag anywhere
// in argv (before "--" or a subcommand token), it marks the flag as provided
// and returns a successful parse immediately, bypassing missing-required-arg
// errors so the caller can unconditionally check `args.help.provided()`.
//
// Default: --help / -h.  Override by supplying template arguments:
//   argon::HelpFlag<>                    // --help, -h  (default)
//   argon::HelpFlag<"version", 'V'>      // --version, -V
//   argon::HelpFlag<"info">              // --info, no short opt
template <StringLiteral LongOpt = "help", char ShortOpt = 'h'>
  requires(detail::is_valid_long_option_name<LongOpt>() &&
           detail::is_valid_short_option_name(ShortOpt))
struct HelpFlag : public Flag<LongOpt, ShortOpt> {
  using Flag<LongOpt, ShortOpt>::Flag;
  // Compile-time sentinel detected by Parser::parse() for early-exit logic.
  static constexpr bool is_help = true;
};

}  // namespace argon
