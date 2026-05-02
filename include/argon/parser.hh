#pragma once

#include <algorithm>
#include <argon/argument.hh>
#include <expected>
#include <format>
#include <functional>
#include <print>
#include <span>
#include <unordered_map>
#include <vector>

namespace argon {

namespace detail {

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
};

inline auto tokenize(std::span<const std::string_view> args,
                     const std::unordered_map<std::string, Nargs>& spec_map)
    -> std::expected<TokenizeResult, std::string> {
  TokenizeResult result;

  for (std::size_t i = 0; i < args.size(); ++i) {
    const auto arg = args[i];

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

    // Collect values up to max, stopping when another option name is encountered
    const auto& nargs_spec = it->second;
    auto& values = result.named[key];
    for (std::size_t count = 0;
         i + 1 < args.size() && (!nargs_spec.max.has_value() || count < *nargs_spec.max) &&
         !spec_map.contains(std::string(args[i + 1]));
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
    auto [... args] = T{};

    if ((... || [&args] -> bool {
          if constexpr (args.type == ArgumentType::command) {
            return hasDuplicateOptions<std::remove_cvref_t<decltype(args.args)>>();
          }
          return false;
        }())) {
      return true;
    }

    std::vector<std::string> options;

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
    program_name_ = args.empty() ? "program" : std::string(args[0]);
    std::println("Parsing arguments for program: {}", program_name_);

    Arguments result;
    auto specMap = this->makeArgumentSpecMap(result);

    auto tokenized = detail::tokenize(args.subspan(1), specMap);
    if (!tokenized) {
      return std::unexpected(tokenized.error());
    }
    auto& [parsedArgs, positional_argument] = *tokenized;

    std::println("Parsed named arguments: {}", parsedArgs);
    std::println("Parsed positional arguments: {}", positional_argument);

    return result;
  }

 private:
  auto getLongOptions(const Arguments& args) const {
    auto ret = std::vector<std::string_view>{};
    auto [... options] = args;
    (..., (ret.push_back(options.longOpt())));
    return ret;
  }

  auto getShortOptions(const Arguments& args) const {
    auto ret = std::vector<char>{};
    auto [... options] = args;
    (..., [&] -> auto {
      const auto short_opt = options.shortOpt();
      if (short_opt != '\0') {
        ret.push_back(short_opt);
      }
    }());
    return ret;
  }

  auto getCommandNames(const Arguments& args) const {
    auto ret = std::vector<std::string_view>{};
    auto [... options] = args;
    (..., [&] -> auto {
      if constexpr (options.type == ArgumentType::command) {
        ret.push_back(options.commandName());
      }
    }());
    return ret;
  }

  [[nodiscard]] auto isArgument(std::string_view arg,
                                const std::vector<std::string_view>& longOptions,
                                const std::vector<char>& shortOptions,
                                const std::vector<std::string_view>& commandNames) const -> bool {
    if (arg.starts_with("--")) {
      auto longOpt = arg.substr(2);
      return std::ranges::find(longOptions, longOpt) != longOptions.end();
    } else if (arg.starts_with("-") && arg.size() == 2) {
      char shortOpt = arg[1];
      return std::ranges::find(shortOptions, shortOpt) != shortOptions.end();
    } else {
      return std::ranges::find(commandNames, arg) != commandNames.end();
    }
  }

  auto makeArgumentSpecMap(const Arguments& args) const {
    auto [... options] = args;

    std::unordered_map<std::string, detail::Nargs> argumentSpecMap;

    (..., [&] -> auto {
      if constexpr (options.type == ArgumentType::option || options.type == ArgumentType::flag) {
        argumentSpecMap.emplace(std::string("--") + options.longOpt(), options.nargs());
        if (options.shortOpt() != '\0') {
          argumentSpecMap.emplace(std::string("-") + std::string(1, options.shortOpt()),
                                  options.nargs());
        }
      }
      if constexpr (options.type == ArgumentType::command) {
        argumentSpecMap.emplace(options.commandName(),
                                detail::Nargs{.min = 0, .max = std::nullopt});
      }
    }());
    return argumentSpecMap;
  }

  auto makeParseMap(const Arguments& args) const {
    auto [... options] = args;

    std::unordered_map<std::string, std::function<std::expected<void, std::string>(
                                        std::span<const std::string_view>)>>
        parseMap;

    (..., [&] -> auto {
      if constexpr (options.type == ArgumentType::option || options.type == ArgumentType::flag) {
        parseMap.emplace(std::string("--") + options.longOpt(),
                         [&options](std::span<const std::string_view> values) -> auto {
                           return options.parse(values);
                         });
        if (options.shortOpt() != '\0') {
          parseMap.emplace(std::string("-") + std::string(1, options.shortOpt()),
                           [&options](std::span<const std::string_view> values) -> auto {
                             return options.parse(values);
                           });
        }
      }
      if constexpr (options.type == ArgumentType::command) {
        parseMap.emplace(options.commandName(),
                         [&options](std::span<const std::string_view> values) -> auto {
                           return options.args.parse(values);
                         });
      }
    }());
    return parseMap;
  }

  std::string program_name_ = "program";
};

}  // namespace argon
