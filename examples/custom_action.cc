#include <charconv>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

#include "cli/argument.hh"
#include "cli/parser.hh"

struct MinutesSinceMidnight {
  int value{};
};

struct HHMMConversion {
  template <class Input>
  static constexpr bool accepts_input =
      cli::deduce_accepts_input<HHMMConversion, Input>;

  template <class Input>
  using after_type = cli::deduce_after_type<HHMMConversion, Input>;

  template <class Input>
  using storage_type = void;

  auto operator()(cli::ActionCtx<void> ctx,
                  cli::ActionResult<std::string_view> input) const
      -> cli::ActionResult<MinutesSinceMidnight> {
    if (!input.have_value()) {
      return cli::fail<MinutesSinceMidnight>(input.error);
    }

    const std::string_view text = input.value;
    const auto colon = text.find(':');
    if (colon == text.npos) {
      return invalid(ctx, text, "expected HH:MM");
    }

    int hours{};
    int minutes{};
    if (!parse_int(text.substr(0, colon), hours) ||
        !parse_int(text.substr(colon + 1), minutes)) {
      return invalid(ctx, text, "hours and minutes must be integers");
    }
    if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59) {
      return invalid(ctx, text, "expected a time between 00:00 and 23:59");
    }

    return cli::ok(MinutesSinceMidnight{.value = hours * 60 + minutes});
  }

 private:
  static auto parse_int(std::string_view text, int& out) -> bool {
    if (text.empty()) return false;
    const auto* first = text.data();
    const auto* last = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(first, last, out);
    return ec == std::errc{} && ptr == last;
  }

  static auto invalid(cli::ActionCtx<void> ctx, std::string_view subject,
                      std::string detail)
      -> cli::ActionResult<MinutesSinceMidnight> {
    return cli::fail<MinutesSinceMidnight>(cli::ParseError{
        .code = cli::ErrorCode::invalid_value,
        .kind = cli::ErrorKind::conversion,
        .position = static_cast<int>(ctx.index),
        .subject = std::string(subject),
        .detail = std::move(detail),
    });
  }
};

inline constexpr auto hhmm = cli::Action<HHMMConversion{}>{};

struct Args {
  cli::Description description{
      "Custom action example using a reusable struct-based conversion."};

  cli::Arg<"start", 's', hhmm | cli::pack::set_once> start{
      {.help = "Start time in HH:MM", .presence = cli::required}};

  cli::Arg<"end", 'e', hhmm | cli::pack::set_once> end{
      {.help = "End time in HH:MM", .presence = cli::required}};
};

auto print_time(MinutesSinceMidnight time) -> void {
  const int hours = time.value / 60;
  const int minutes = time.value % 60;
  if (hours < 10) std::cout << '0';
  std::cout << hours << ':';
  if (minutes < 10) std::cout << '0';
  std::cout << minutes;
}

auto main(int argc, char* argv[]) -> int {
  const auto args = cli::parse_or_exit<Args>(argc, argv);

  std::cout << "start: ";
  print_time(*args.start);
  std::cout << " (" << args.start->value << " minutes)\n";

  std::cout << "end: ";
  print_time(*args.end);
  std::cout << " (" << args.end->value << " minutes)\n";

  return 0;
}
