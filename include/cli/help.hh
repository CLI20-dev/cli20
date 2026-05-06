#pragma once

#include <algorithm>
#include <format>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "cli/argument.hh"
#include "cli/color.hh"
#include "cli/meta.hh"

namespace cli {

namespace detail {

template <class T>
struct MetavarName {
  static constexpr std::string_view value = "value";
};

template <>
struct MetavarName<std::string> {
  static constexpr std::string_view value = "string";
};

template <>
struct MetavarName<bool> {
  static constexpr std::string_view value = "bool";
};

template <>
struct MetavarName<float> {
  static constexpr std::string_view value = "float";
};

template <>
struct MetavarName<double> {
  static constexpr std::string_view value = "double";
};

template <>
struct MetavarName<std::filesystem::path> {
  static constexpr std::string_view value = "path";
};

template <std::signed_integral T>
struct MetavarName<T> {
  static constexpr std::string_view value = "int";
};

template <std::unsigned_integral T>
struct MetavarName<T> {
  static constexpr std::string_view value = "uint";
};

template <class T>
struct UnwrapStorage {
  using type = std::remove_cvref_t<T>;
  static constexpr bool variadic = false;
};

template <class T>
struct UnwrapStorage<std::optional<T>> {
  using type = std::remove_cvref_t<T>;
  static constexpr bool variadic = false;
};

template <class T>
struct UnwrapStorage<std::vector<T>> {
  using type = std::remove_cvref_t<T>;
  static constexpr bool variadic = true;
};

template <class T>
using unwrap_storage_t = typename UnwrapStorage<std::remove_cvref_t<T>>::type;

template <class T>
[[nodiscard]]
auto metavar_for() -> std::string {
  using Storage = std::remove_cvref_t<T>;
  using Base = unwrap_storage_t<Storage>;
  std::string out = std::format("<{}>", MetavarName<Base>::value);
  if constexpr (UnwrapStorage<Storage>::variadic) {
    out.insert(out.size() - 1, "...");
  }
  return out;
}

template <class T>
[[nodiscard]]
auto command_usage_suffix(T& value) -> std::string {
  std::string out;
  bool has_options = false;
  bool has_positionals = false;
  bool has_commands = false;

  std::apply(
      [&](auto&... fields) {
        (
            [&] {
              using F = std::remove_cvref_t<decltype(fields)>;
              if constexpr (std::derived_from<F, OptionTag>) {
                has_options = true;
              } else if constexpr (std::derived_from<F, PositionalTag>) {
                has_positionals = true;
              } else if constexpr (std::derived_from<F, CommandTag>) {
                has_commands = true;
              }
            }(),
            ...);
      },
      as_tuple(value));

  if (has_options) {
    out += " [options]";
  }
  if (has_positionals) {
    out += " [args]";
  }
  if (has_commands) {
    out += " [command]";
  }
  return out;
}

template <class T>
[[nodiscard]]
auto field_usage_label() -> std::string {
  using F = std::remove_cvref_t<T>;
  if constexpr (std::derived_from<F, OptionTag>) {
    std::string out;
    if constexpr (F::short_name != '\0') {
      out += std::format("-{}, ", F::short_name);
    }
    out += std::format("--{}", F::name.view());
    if constexpr (!(F::nargs.min == 0 && F::nargs.max == 0)) {
      out += " ";
      out += metavar_for<typename F::value_type>();
    }
    return out;
  } else if constexpr (std::derived_from<F, PositionalTag>) {
    return metavar_for<typename F::value_type>();
  } else if constexpr (std::derived_from<F, CommandTag>) {
    return std::string(F::commandName());
  } else {
    return std::string{};
  }
}

inline auto split_lines(std::string_view text) -> std::vector<std::string> {
  std::vector<std::string> lines;
  std::size_t start = 0;
  while (start <= text.size()) {
    const auto end = text.find('\n', start);
    if (end == std::string_view::npos) {
      lines.emplace_back(text.substr(start));
      break;
    }
    lines.emplace_back(text.substr(start, end - start));
    start = end + 1;
  }
  if (lines.empty()) {
    lines.emplace_back();
  }
  return lines;
}

inline auto append_wrapped_description(std::string& out,
                                       std::string_view description,
                                       std::size_t indent) -> void {
  if (description.empty()) {
    out += '\n';
    return;
  }

  auto lines = split_lines(description);
  out += lines.front();
  out += '\n';
  for (std::size_t i = 1; i < lines.size(); ++i) {
    out.append(indent, ' ');
    out += lines[i];
    out += '\n';
  }
}

template <class T, class Pred, class Render>
auto append_section(std::string& out, std::string_view title,
                    std::string_view heading_color, std::string_view reset_color,
                    T& value, Pred pred, Render render) -> void {
  std::vector<std::string> rows;
  std::size_t width = 0;

  std::apply(
      [&](auto&... fields) {
        (
            [&] {
              using F = std::remove_cvref_t<decltype(fields)>;
              if constexpr (pred.template operator()<F>()) {
                const auto label = field_usage_label<F>();
                width = std::max(width, label.size());
                rows.push_back(render(fields, label));
              }
            }(),
            ...);
      },
      as_tuple(value));

  if (rows.empty()) {
    return;
  }

  out += '\n';
  out += heading_color;
  out += title;
  out += reset_color;
  out += '\n';
  for (auto& row : rows) {
    out += row;
  }
}

template <class Field>
auto render_help_row(Field& field, std::string_view label, std::size_t width)
    -> std::string {
  std::string out = "  ";
  out += label;
  if (!field.help.empty()) {
    out.append(width - label.size() + 2, ' ');
    append_wrapped_description(out, field.help, width + 4);
  } else {
    out += '\n';
  }
  return out;
}

template <class T>
auto find_description(T& value) -> std::string_view {
  std::string_view description;
  std::apply(
      [&](auto&... fields) {
        (
            [&] {
              using F = std::remove_cvref_t<decltype(fields)>;
              if constexpr (std::derived_from<F, DescriptionTag>) {
                if (description.empty()) {
                  description = fields;
                }
              }
            }(),
            ...);
      },
      as_tuple(value));
  return description;
}

template <class T>
auto format_help_impl(T& value, std::string_view program_name,
                      ColorMode color_mode, bool recurse,
                      std::string_view command_path) -> std::string {
  const auto style = resolveColor(color_mode);
  const std::string heading =
      std::string(style.bold()) + std::string(style.underline());
  const std::string option_color = std::string(style.bold());
  const std::string reset = std::string(style.reset());

  std::string full_program = std::string(program_name);
  if (!command_path.empty()) {
    full_program += " ";
    full_program += command_path;
  }

  std::string out =
      std::format("Usage: {}{}\n", full_program, command_usage_suffix(value));

  if (const auto description = find_description(value); !description.empty()) {
    out += '\n';
    out += description;
    out += '\n';
  }

  std::vector<std::pair<std::string, std::string_view>> option_rows;
  std::vector<std::pair<std::string, std::string_view>> positional_rows;
  std::vector<std::pair<std::string, std::string_view>> command_rows;
  std::size_t option_width = 0;
  std::size_t positional_width = 0;
  std::size_t command_width = 0;

  std::apply(
      [&](auto&... fields) {
        (
            [&] {
              using F = std::remove_cvref_t<decltype(fields)>;
              if constexpr (std::derived_from<F, OptionTag>) {
                auto label = field_usage_label<F>();
                option_width = std::max(option_width, label.size());
                option_rows.emplace_back(std::move(label), fields.help);
              } else if constexpr (std::derived_from<F, PositionalTag>) {
                auto label = field_usage_label<F>();
                positional_width = std::max(positional_width, label.size());
                positional_rows.emplace_back(std::move(label), fields.help);
              } else if constexpr (std::derived_from<F, CommandTag>) {
                auto label = field_usage_label<F>();
                command_width = std::max(command_width, label.size());
                command_rows.emplace_back(std::move(label), fields.helpText());
              }
            }(),
            ...);
      },
      as_tuple(value));

  auto append_rows =
      [&](std::string_view title,
          const std::vector<std::pair<std::string, std::string_view>>& rows,
          std::size_t width) {
        if (rows.empty()) {
          return;
        }
        out += '\n';
        out += heading;
        out += title;
        out += reset;
        out += '\n';
        for (const auto& [label, description] : rows) {
          out += "  ";
          out += option_color;
          out += label;
          out += reset;
          if (!description.empty()) {
            out.append(width - label.size() + 2, ' ');
            append_wrapped_description(out, description, width + 4);
          } else {
            out += '\n';
          }
        }
      };

  append_rows("Options:", option_rows, option_width);
  append_rows("Positional arguments:", positional_rows, positional_width);
  append_rows("Commands:", command_rows, command_width);

  if (recurse && !command_rows.empty()) {
    std::apply(
        [&](auto&... fields) {
          (
              [&] {
                using F = std::remove_cvref_t<decltype(fields)>;
                if constexpr (std::derived_from<F, CommandTag>) {
                  out += '\n';
                  out += format_help_impl<typename F::argument_type>(
                      static_cast<typename F::argument_type&>(fields),
                      program_name, color_mode, true, F::commandName());
                }
              }(),
              ...);
        },
        as_tuple(value));
  }

  return out;
}

}  // namespace detail

template <ArgumentSpec T>
auto formatHelp(T& value, std::string_view program_name = "program",
                ColorMode color_mode = ColorMode::auto_, bool recurse = false)
    -> std::string {
  return detail::format_help_impl<T>(value, program_name, color_mode, recurse,
                                     {});
}

}  // namespace cli
