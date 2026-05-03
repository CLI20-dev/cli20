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

namespace argon {

namespace detail {

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
    -> std::expected<TokenizeResult, std::string> {
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
            return std::unexpected(std::format("option '{}' specified multiple times", key));
          }
          result.named[key].push_back(val);
        } else {
          result.positional.push_back(arg);
        }
        continue;
      }
    }

    // Look up the token in spec_map
    const auto it = spec_map.find(std::string(arg));
    if (it == spec_map.end()) {
      result.positional.push_back(arg);
      continue;
    }

    const auto key = std::string(arg);
    if (result.named.contains(key)) {
      return std::unexpected(std::format("option '{}' specified multiple times", key));
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
      return std::unexpected(std::format("option '{}' requires at least {} argument(s), but got {}",
                                         key, nargs_spec.min, values.size()));
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
  [[nodiscard]]
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
  auto parse(int argc, char* argv[]) -> std::expected<Arguments, std::string> {
    std::vector<std::string_view> args;
    args.reserve(static_cast<std::size_t>(argc));
    for (char* arg : std::span<char*>{argv, static_cast<std::size_t>(argc)}) {
      args.emplace_back(arg);
    }
    return parse(args);
  }

  auto parse(std::span<const std::string_view> args) -> std::expected<Arguments, std::string> {
    Arguments result;
    if (auto r = parse(args, result); !r) {
      return std::unexpected(r.error());
    }
    return result;
  }

  auto parse(std::span<const std::string_view> args, Arguments& out) -> std::expected<void, std::string> {
    program_name_ = args.empty() ? "program" : std::string(args[0]);

    const auto rest = args.size() > 1 ? args.subspan(1) : std::span<const std::string_view>{};

    auto specMap = makeOptionSpecMap(out);
    auto cmdNames = makeCommandNamesSet(out);

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
    std::string pos_error;
    std::string missing;
    (..., [&] -> auto {
      if constexpr (opts.type == ArgumentType::positional) {
        if (pos_idx < tokenized->positional.size()) {
          const auto sv = tokenized->positional[pos_idx++];
          if constexpr (requires { opts.parse(sv); }) {
            if (pos_error.empty()) {
              auto r = opts.parse(sv);
              if (!r) {
                pos_error = r.error().message();
              } else {
                opts.markProvided();
              }
            }
          }
        } else {
          if constexpr (requires { opts.isRequired(); }) {
            if (opts.isRequired() && pos_error.empty()) {
              pos_error = std::format("required positional argument (position {}) was not provided",
                                      pos_idx + 1);
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
    if (!pos_error.empty()) {
      return std::unexpected(pos_error);
    }
    if (!missing.empty()) {
      return std::unexpected(std::format("required option '{}' was not provided", missing));
    }

    // Recursively parse sub-command if one was encountered
    if (tokenized->command_tail) {
      std::string cmd_error;
      (..., [&] -> auto {
        if constexpr (opts.type == ArgumentType::command) {
          const auto& tail = *tokenized->command_tail;
          if (!tail.empty() && tail[0] == opts.commandName() && cmd_error.empty()) {
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
      if (!cmd_error.empty()) {
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

    std::unordered_map<std::string, std::function<std::expected<void, std::string>(
                                        std::span<const std::string_view>)>>
        parseMap;

    (..., [&] -> auto {
      if constexpr (options.type == ArgumentType::option) {
        auto make_fn =
            [&options](
                std::span<const std::string_view> values) -> std::expected<void, std::string> {
          auto r = options.parse(values);
          if (!r) {
            return std::unexpected(r.error().message());
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
            [&options](std::span<const std::string_view>) -> std::expected<void, std::string> {
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

  std::string program_name_ = "program";
};

// ---- Free-function syntax sugar ----

template <class Arguments>
[[nodiscard]]
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
auto parse(int argc, char* argv[]) -> std::expected<Arguments, std::string> {
  return Parser<Arguments>{}.parse(argc, argv);
}

template <class Arguments>
[[nodiscard]]
auto parse(std::span<const std::string_view> args) -> std::expected<Arguments, std::string> {
  return Parser<Arguments>{}.parse(args);
}

}  // namespace argon
