#pragma once

#include <algorithm>
#include <argon/argument.hh>
#include <expected>
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

    (..., [&options, &args] -> auto {
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

    std::vector<std::string_view> positional_argument = {};
    auto parsedArgs = std::unordered_map<std::string, std::vector<std::string_view>>{};
    auto longOptions = getLongOptions(result);
    auto shortOptions = getShortOptions(result);
    auto commandNames = getCommandNames(result);

    for (std::size_t i = 1; i < args.size(); ++i) {
      auto it = specMap.find(std::string(args[i]));
      if (it == specMap.end()) {
        positional_argument.push_back(args[i]);
        continue;
      }
    }

    // [[maybe_unused]] auto& [... optionLists] = result;

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

    (..., [&argumentSpecMap, &options] -> auto {
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

  std::string program_name_ = "program";
};

}  // namespace argon
