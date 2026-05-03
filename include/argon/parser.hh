#pragma once

#include <algorithm>
#include <argon/argument.hh>
#include <expected>
#include <format>
#include <functional>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "argon/color.hh"
#include "argon/error.hh"

namespace argon {

namespace detail {

// Returns a human-readable type metavar string for use in help output.
template <typename T>
auto typeMetavar() -> std::string {
  if constexpr (std::same_as<T, std::string>) {
    return "string";
  } else if constexpr (std::same_as<T, bool>) {
    return "bool";
  } else if constexpr (std::same_as<T, float>) {
    return "float";
  } else if constexpr (std::same_as<T, double>) {
    return "double";
  } else if constexpr (std::unsigned_integral<T> && !std::same_as<T, bool>) {
    return "uint";
  } else if constexpr (std::integral<T>) {
    return "int";
  } else if constexpr (requires { typename T::value_type; }) {
    return typeMetavar<typename T::value_type>() + "...";
  } else {
    return "value";
  }
}

template <class Argument>
constexpr auto castBaseIfCommand(Argument& arg) -> auto& {
  if constexpr (requires {
                  Argument::type;
                  Argument::type == ArgumentType::command;
                }) {
    return static_cast<Argument::args_type&>(arg);
  } else {
    return arg;
  }
}

// Returns the text of any Description member found in SubArgs, or empty.
template <class SubArgs>
auto getStructDescription() -> std::string_view {
  SubArgs sub{};
  auto& [... fields] = sub;
  std::string_view desc;
  (..., [&] -> auto {
    if constexpr (fields.type == ArgumentType::description) {
      desc = fields.text();
    }
  }());
  return desc;
}

template <class Arguments>
struct isValidArgumentsType {
  static constexpr bool value = [] -> bool {
    if constexpr (!std::derived_from<std::remove_cvref_t<Arguments>, ArgumentTag>) {
      auto&& [... options] = Arguments{};
      return (... && (std::derived_from<std::remove_cvref_t<decltype(options)>, ArgumentTag>));
    }
    return false;
  }();
};

struct TokenizeResult {
  std::unordered_map<std::string, std::vector<std::string_view>> named;
  std::vector<std::string_view> positional;
  // Set when a command name is encountered; points from that token to end of args.
  std::optional<std::span<const std::string_view>> command_tail;
};

// spec_map      : option/flag keys → nargs  (does NOT contain command names)
// command_names : set of bare command name strings (no "--" prefix)
inline auto tokenize(std::span<const std::string_view> args,
                     const std::unordered_map<std::string, Nargs>& spec_map,
                     const std::unordered_set<std::string>& command_names = {})
    -> std::expected<TokenizeResult, ParseError> {
  TokenizeResult result;

  for (std::size_t i = 0; i < args.size(); ++i) {
    const auto arg = args[i];

    // "--" end-of-options separator: everything after is positional
    if (arg == "--") {
      for (std::size_t j = i + 1; j < args.size(); ++j) {
        result.positional.push_back(args[j]);
      }
      break;
    }

    // Command name: stop parsing at this level; hand off remainder to sub-parser
    if (command_names.contains(std::string(arg))) {
      result.command_tail = args.subspan(i);
      break;
    }

    // --foo=bar syntax: only recognized when nargs is exact<1>
    if (arg.starts_with("--")) {
      if (const auto eq = arg.find('='); eq != std::string_view::npos) {
        const auto key = std::string(arg.substr(0, eq));
        const auto val = arg.substr(eq + 1);
        const auto it = spec_map.find(key);
        if (it != spec_map.end() && it->second.min == 1 &&
            it->second.max == std::optional<std::size_t>{1}) {
          if (result.named.contains(key)) {
            return std::unexpected(ParseError{.code = ErrorCode::duplicate_argument,
                                              .kind = ErrorKind::parse,
                                              .position = static_cast<int>(i),
                                              .subject = key,
                                              .detail = "option specified multiple times"});
          }
          result.named[key].push_back(val);
        } else {
          return std::unexpected(ParseError{.code = ErrorCode::unknown_option,
                                            .kind = ErrorKind::parse,
                                            .position = static_cast<int>(i),
                                            .subject = key});
        }
        continue;
      }
    }

    // Look up the token in spec_map
    const auto it = spec_map.find(std::string(arg));
    if (it == spec_map.end()) {
      // Arguments that look like options (start with '-') but are not in the spec are errors.
      // Bare words without a leading '-' are positional arguments.
      // To pass a '--'-prefixed string as a positional, use the '--' end-of-options separator.
      if (arg.size() > 1 && arg[0] == '-') {
        return std::unexpected(ParseError{.code = ErrorCode::unknown_option,
                                          .kind = ErrorKind::parse,
                                          .position = static_cast<int>(i),
                                          .subject = std::string(arg)});
      }
      result.positional.push_back(arg);
      continue;
    }

    const auto key = std::string(arg);
    if (result.named.contains(key)) {
      return std::unexpected(ParseError{.code = ErrorCode::duplicate_argument,
                                        .kind = ErrorKind::parse,
                                        .position = static_cast<int>(i),
                                        .subject = key,
                                        .detail = "option specified multiple times"});
    }

    // Collect values up to max, stopping at the next option/command/separator
    const auto& nargs_spec = it->second;
    auto& values = result.named[key];
    for (std::size_t count = 0;
         i + 1 < args.size() && (!nargs_spec.max.has_value() || count < *nargs_spec.max) &&
         args[i + 1] != "--" && !spec_map.contains(std::string(args[i + 1])) &&
         !command_names.contains(std::string(args[i + 1]));
         ++count) {
      values.push_back(args[++i]);
    }

    if (values.size() < nargs_spec.min) {
      return std::unexpected(ParseError{
          .code = ErrorCode::missing_value,
          .kind = ErrorKind::parse,
          .position = static_cast<int>(i),
          .subject = key,
          .detail = std::format("option requires at least {} value(s), but only {} provided",
                                nargs_spec.min, values.size())});
    }
  }

  return result;
}

}  // namespace detail

template <class Arguments>
class Parser {
  static_assert(detail::isValidArgumentsType<Arguments>::value,
                "Arguments must be a struct or nested struct containing only Argument types");

 private:
  template <class T = Arguments>
  static consteval auto hasDuplicateOptions() -> bool {
    std::vector<std::string> options;
    auto t = T{};
    auto [... args] = detail::castBaseIfCommand(t);

    if ((... || [&args] -> bool {
          if constexpr (args.type == ArgumentType::command) {
            return hasDuplicateOptions<std::remove_cvref_t<decltype(args)>>();
          }
          return false;
        }())) {
      return true;
    }

    (..., [&] -> auto {
      if constexpr (args.type == ArgumentType::option || args.type == ArgumentType::flag) {
        options.emplace_back(std::string("--") + args.longOpt());
        if (args.shortOpt() != '\0') {
          options.emplace_back(std::string("-") + args.shortOpt());
        }
      }
      if constexpr (args.type == ArgumentType::command) {
        options.emplace_back(std::string(args.commandName()));
      }
    }());

    std::ranges::sort(options);
    return std::ranges::adjacent_find(options) != options.end();
  }

  static_assert(!hasDuplicateOptions(),
                "Arguments must not contain duplicate long or short options");

 public:
  [[nodiscard]] auto formatHelp(ColorMode color = ColorMode::auto_) const -> std::string {
    return formatHelpImpl(color, false);
  }

  [[nodiscard]] auto formatHelp(RecurseHelpTag /*tag*/) const -> std::string {
    return formatHelpImpl(ColorMode::auto_, true);
  }

  [[nodiscard]] auto formatHelp(ColorMode color, RecurseHelpTag /*tag*/) const -> std::string {
    return formatHelpImpl(color, true);
  }

 private:
  [[nodiscard]] auto formatHelpImpl(ColorMode color, bool recurse) const -> std::string {
    const detail::AnsiStyle ansi = detail::resolveColor(color);

    Arguments defaults{};
    auto& [... opts] = defaults;

    // Each entry stores the plain-text left column (used for alignment measurement)
    // and the description.  ANSI codes are applied when printing, not stored here,
    // so that col-width arithmetic works on visual character counts.
    struct Entry {
      std::string left;
      std::string_view desc;
    };
    std::vector<Entry> options_section;
    std::vector<Entry> positionals_section;
    std::vector<Entry> commands_section;
    bool has_options = false;
    bool has_commands = false;

    (..., [&] -> auto {
      using OptT = std::remove_cvref_t<decltype(opts)>;
      if constexpr (opts.type == ArgumentType::option) {
        std::string left;
        if (opts.shortOpt() != '\0') {
          left += '-';
          left += opts.shortOpt();
          left += ", ";
        } else {
          left += "    ";
        }
        left += "--";
        left += opts.longOpt();
        left += " <";
        left += detail::typeMetavar<typename OptT::value_type>();
        left += '>';
        options_section.push_back({std::move(left), opts.description()});
        has_options = true;
      }
      if constexpr (opts.type == ArgumentType::flag) {
        std::string left;
        if (opts.shortOpt() != '\0') {
          left += '-';
          left += opts.shortOpt();
          left += ", ";
        } else {
          left += "    ";
        }
        left += "--";
        left += opts.longOpt();
        options_section.push_back({std::move(left), opts.description()});
        has_options = true;
      }
      if constexpr (opts.type == ArgumentType::positional) {
        using VT = typename OptT::value_type;
        std::string mv = "<";
        mv += detail::typeMetavar<VT>();
        mv += '>';
        positionals_section.push_back({std::move(mv), opts.description()});
      }
      if constexpr (opts.type == ArgumentType::command) {
        // Use the Command's own description; fall back to a Description member in SubArgs.
        std::string_view cmd_desc = opts.description();
        if (cmd_desc.empty()) {
          using SubArgs = std::remove_cvref_t<decltype(detail::castBaseIfCommand(opts))>;
          cmd_desc = detail::getStructDescription<SubArgs>();
        }
        commands_section.push_back({std::string(opts.commandName()), cmd_desc});
        has_commands = true;
      }
    }());

    // Extract struct-level description (if any)
    std::string_view struct_desc;
    (..., [&] -> auto {
      if constexpr (opts.type == ArgumentType::description) {
        struct_desc = opts.text();
      }
    }());

    // Usage line — "Usage:" in bold, rest plain
    std::string out;
    out += ansi.bold();
    out += "Usage:";
    out += ansi.reset();
    out += " ";
    out += program_name_;
    if (has_options) out += " [options]";
    for (const auto& e : positionals_section) out += " " + e.left;
    if (has_commands) out += " [command]";
    out += '\n';

    if (!struct_desc.empty()) {
      out += '\n';
      out += struct_desc;
      out += '\n';
    }

    // Compute alignment column from plain-text widths only (ANSI codes are
    // zero-width and must not be counted here).
    // col = 2 (indent) + max_left_width + 2 (gap)
    std::size_t max_left = 0;
    for (const auto& e : options_section) max_left = std::max(max_left, e.left.size());
    for (const auto& e : positionals_section) max_left = std::max(max_left, e.left.size());
    for (const auto& e : commands_section) max_left = std::max(max_left, e.left.size());
    const std::size_t col = max_left + 4;  // 2 indent + 2 gap

    auto append_section = [&](std::string_view header, const std::vector<Entry>& entries) {
      if (entries.empty()) return;
      out += '\n';
      out += ansi.bold();
      out += ansi.underline();
      out += header;
      out += ansi.reset();
      out += ":\n";
      // Continuation lines after a user-inserted '\n' are indented to `col`
      // so they align with the first line of the description.
      const std::string cont_indent(col, ' ');
      for (const auto& e : entries) {
        out += "  ";
        out += ansi.bold();
        out += e.left;
        out += ansi.reset();
        if (!e.desc.empty()) {
          out += std::string(col - 2 - e.left.size(), ' ');
          std::string_view rest = e.desc;
          while (true) {
            const auto nl = rest.find('\n');
            if (nl == std::string_view::npos) { out += rest; break; }
            out += rest.substr(0, nl);
            out += '\n';
            out += cont_indent;
            rest = rest.substr(nl + 1);
          }
        }
        out += '\n';
      }
    };

    append_section("Options", options_section);
    append_section("Positional arguments", positionals_section);
    append_section("Commands", commands_section);

    // Recursively append each sub-command's help text
    if (recurse) {
      (..., [&] -> auto {
        if constexpr (opts.type == ArgumentType::command) {
          using SubArgs = std::remove_cvref_t<decltype(detail::castBaseIfCommand(opts))>;
          Parser<SubArgs> sub_parser;
          sub_parser.program_name_ = program_name_ + " " + std::string(opts.commandName());

          // ─── <name> ─── separator  (U+2500 BOX DRAWINGS LIGHT HORIZONTAL)
          constexpr std::string_view dash = "\xe2\x94\x80";  // UTF-8 for ─
          constexpr std::size_t rule_width = 48;
          const std::string name_part =
              std::string(" ") + std::string(opts.commandName()) + " ";
          const std::size_t left_count = 3;
          const std::size_t right_count =
              rule_width > left_count + name_part.size()
                  ? rule_width - left_count - name_part.size()
                  : 3;
          auto repeat_dash = [&](std::size_t n) {
            for (std::size_t k = 0; k < n; ++k) out += dash;
          };
          out += '\n';
          out += ansi.bold();
          repeat_dash(left_count);
          out += name_part;
          repeat_dash(right_count);
          out += ansi.reset();
          {
            std::string_view sep_desc = opts.description();
            if (sep_desc.empty()) sep_desc = detail::getStructDescription<SubArgs>();
            if (!sep_desc.empty()) {
              out += "  ";
              out += sep_desc;
            }
          }
          out += '\n';
          out += sub_parser.formatHelpImpl(color, true);
        }
      }());
    }

    return out;
  }

 public:
  [[nodiscard]]
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
  auto parse(int argc, char* argv[]) -> std::expected<Arguments, ParseError> {
    std::vector<std::string_view> args;
    args.reserve(static_cast<std::size_t>(argc));
    for (char* arg : std::span<char*>{argv, static_cast<std::size_t>(argc)}) {
      args.emplace_back(arg);
    }
    return parse(args);
  }

  auto parse(std::span<const std::string_view> args) -> std::expected<Arguments, ParseError> {
    Arguments result;
    if (auto r = parse(args, result); !r) {
      return std::unexpected(r.error());
    }
    return result;
  }

  auto parse(std::span<const std::string_view> args, Arguments& out)
      -> std::expected<void, ParseError> {
    program_name_ = args.empty() ? "program" : std::string(args[0]);

    const auto rest = args.size() > 1 ? args.subspan(1) : std::span<const std::string_view>{};

    auto specMap = makeOptionSpecMap(out);
    auto cmdNames = makeCommandNamesSet(out);

    if (checkHelpFlag(rest, out, cmdNames)) {
      return {};
    }

    auto tokenized = detail::tokenize(rest, specMap, cmdNames);
    if (!tokenized) {
      return std::unexpected(tokenized.error());
    }

    auto parseMap = makeParseMap(out);

    for (const auto& [key, values] : tokenized->named) {
      const auto it = parseMap.find(key);
      if (it != parseMap.end()) {
        auto r = it->second(values);
        if (!r) {
          return std::unexpected(r.error());
        }
      }
    }

    // Assign positionals and check required options
    auto& [... opts] = out;
    std::size_t pos_idx = 0;
    ParseError pos_error;
    std::string missing;
    (..., [&] -> auto {
      if constexpr (opts.type == ArgumentType::positional) {
        if (pos_idx < tokenized->positional.size()) {
          const auto sv = tokenized->positional[pos_idx++];
          if constexpr (requires { opts.parse(sv); }) {
            if (!pos_error.hasError()) {
              auto r = opts.parse(sv);
              if (!r) {
                pos_error = r.error();
              } else {
                if constexpr (requires { opts.validate(); }) {
                  auto v = opts.validate();
                  if (!v) { pos_error = v.error(); }
                }
                if (!pos_error.hasError()) opts.markProvided();
              }
            }
          }
        } else {
          if constexpr (requires { opts.isRequired(); }) {
            if (opts.isRequired() && !pos_error.hasError()) {
              pos_error = ParseError{
                  .code = ErrorCode::missing_value,
                  .kind = ErrorKind::parse,
                  .position = static_cast<int>(args.size()),
                  .subject = std::format("positional argument at position {}", pos_idx + 1)};
            }
          }
          ++pos_idx;
        }
      }
      if constexpr (opts.type == ArgumentType::option) {
        if (opts.isRequired() && !opts.provided() && missing.empty()) {
          missing = std::string("--") + std::string(opts.longOpt());
        }
      }
    }());
    if (tokenized->positional.size() > pos_idx) {
      return std::unexpected(ParseError{
          .code = ErrorCode::unexpected_argument,
          .kind = ErrorKind::parse,
          .subject = std::string(tokenized->positional[pos_idx]),
          .detail = std::format("{} positional argument(s) provided but only {} expected",
                                tokenized->positional.size(), pos_idx)});
    }
    if (pos_error.hasError()) {
      return std::unexpected(pos_error);
    }
    if (!missing.empty()) {
      return std::unexpected(ParseError{.code = ErrorCode::missing_value,
                                        .kind = ErrorKind::parse,
                                        .position = static_cast<int>(args.size()),
                                        .subject = missing,
                                        .detail = "required option was not provided"});
    }

    // Recursively parse sub-command if one was encountered
    if (tokenized->command_tail) {
      ParseError cmd_error;
      (..., [&] -> auto {
        if constexpr (opts.type == ArgumentType::command) {
          const auto& tail = *tokenized->command_tail;
          if (!tail.empty() && tail[0] == opts.commandName() && !cmd_error.hasError()) {
            using SubArgs = std::remove_cvref_t<decltype(detail::castBaseIfCommand(opts))>;
            Parser<SubArgs> sub_parser;
            if (auto r = sub_parser.parse(tail, detail::castBaseIfCommand(opts)); !r) {
              cmd_error = r.error();
            } else {
              opts.markProvided();
            }
          }
        }
      }());
      if (cmd_error.hasError()) {
        cmd_error.position += static_cast<int>(args.size() - tokenized->command_tail->size());
        return std::unexpected(cmd_error);
      }
    }

    return {};
  }

 private:
  // Build a spec map for options and flags only (commands are handled separately).
  auto makeOptionSpecMap(const Arguments& args) const {
    auto [... options] = args;

    std::unordered_map<std::string, detail::Nargs> specMap;

    (..., [&] -> auto {
      if constexpr (options.type == ArgumentType::option || options.type == ArgumentType::flag) {
        specMap.emplace(std::string("--") + options.longOpt(), options.nargs());
        if (options.shortOpt() != '\0') {
          specMap.emplace(std::string("-") + std::string(1, options.shortOpt()), options.nargs());
        }
      }
    }());
    return specMap;
  }

  // Collect bare command name strings (no "--" prefix).
  auto makeCommandNamesSet(const Arguments& args) const {
    auto [... options] = args;

    std::unordered_set<std::string> cmdNames;

    (..., [&] -> auto {
      if constexpr (options.type == ArgumentType::command) {
        cmdNames.emplace(std::string(options.commandName()));
      }
    }());
    return cmdNames;
  }

  // Build a map from option key (e.g. "--foo", "-f") to a callable that parses
  // the tokenized values into the corresponding field of `args` and marks it as provided.
  // `args` must remain alive for as long as the returned map is used.
  auto makeParseMap(Arguments& args) {
    auto& [... options] = args;

    std::unordered_map<std::string, std::function<std::expected<void, ParseError>(
                                        std::span<const std::string_view>)>>
        parseMap;

    (..., [&] -> auto {
      if constexpr (options.type == ArgumentType::option) {
        auto make_fn =
            [&options](
                std::span<const std::string_view> values) -> std::expected<void, ParseError> {
          auto r = options.parse(values);
          if (!r) return std::unexpected(r.error());
          if constexpr (requires { options.validate(); }) {
            auto v = options.validate();
            if (!v) return std::unexpected(v.error());
          }
          options.markProvided();
          return {};
        };
        parseMap.emplace(std::string("--") + options.longOpt(), make_fn);
        if (options.shortOpt() != '\0') {
          parseMap.emplace(std::string("-") + std::string(1, options.shortOpt()), make_fn);
        }
      }
      if constexpr (options.type == ArgumentType::flag) {
        auto make_fn =
            [&options](std::span<const std::string_view>) -> std::expected<void, ParseError> {
          options.markProvided();
          return {};
        };
        parseMap.emplace(std::string("--") + options.longOpt(), make_fn);
        if (options.shortOpt() != '\0') {
          parseMap.emplace(std::string("-") + std::string(1, options.shortOpt()), make_fn);
        }
      }
    }());
    return parseMap;
  }

  // Scan raw args for a HelpFlag before full tokenization.
  // Stops scanning at "--" or at any command name token.
  // Returns true if a HelpFlag was found and marks it as provided.
  auto checkHelpFlag(std::span<const std::string_view> rest, Arguments& out,
                     const std::unordered_set<std::string>& cmd_names) -> bool {
    auto& [... opts] = out;

    // Collect the long and short option strings for each HelpFlag field
    bool found = false;
    (..., [&] -> auto {
      if constexpr (requires { std::remove_cvref_t<decltype(opts)>::is_help; }) {
        const std::string long_key = std::string("--") + std::string(opts.longOpt());
        const char short_ch = opts.shortOpt();
        const std::string short_key = short_ch != '\0' ? std::string("-") + short_ch : "";

        for (const auto& tok : rest) {
          if (tok == "--") break;
          if (cmd_names.contains(std::string(tok))) break;
          if (tok == long_key || (!short_key.empty() && tok == short_key)) {
            opts.markProvided();
            found = true;
            break;
          }
        }
      }
    }());
    return found;
  }

  template <class>
  friend class Parser;

  std::string program_name_ = "program";
};

// ---- Free-function syntax sugar ----

template <class Arguments>
[[nodiscard]]
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
auto parse(int argc, char* argv[]) -> std::expected<Arguments, ParseError> {
  return Parser<Arguments>{}.parse(argc, argv);
}

template <class Arguments>
[[nodiscard]]
auto parse(std::span<const std::string_view> args) -> std::expected<Arguments, ParseError> {
  return Parser<Arguments>{}.parse(args);
}

}  // namespace argon
