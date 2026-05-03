#pragma once

#include <argon/argument.hh>

namespace argon {

template <StringLiteral LongOpt, char ShortOpt>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
struct Flag : ArgumentTag {
  static constexpr auto type = ArgumentType::flag;

  [[nodiscard]] constexpr auto provided() const noexcept -> bool { return provided_; }

 protected:
  [[nodiscard]] static constexpr auto longOpt() -> std::string_view { return LongOpt.view(); }
  [[nodiscard]] static constexpr auto shortOpt() -> char { return ShortOpt; }

  template <class>
  friend class Parser;

  [[nodiscard]] auto nargs() const noexcept -> detail::Nargs { return nargs_; }
  constexpr auto markProvided() noexcept -> void { provided_ = true; }

 private:
  detail::Nargs nargs_ = nargs::none;
  bool provided_ = false;
};

template <StringLiteral LongOpt, char ShortOpt = '\0'>
  requires(detail::IsValidLongOpt<LongOpt>() && detail::IsValidShortOpt(ShortOpt))
using FlagArg = Flag<LongOpt, ShortOpt>;

}  // namespace argon
