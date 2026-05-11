#pragma once

#include <algorithm>
#include <format>
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

/**
 * @brief Traits type that maps a C++ type to its help metavar name.
 *
 * The primary template uses `"value"` as a fallback. Specializations
 * provide names such as `"string"`, `"int"`, `"path"`, etc.
 *
 * @tparam T The value type to look up.
 */
template <class T>
struct MetavarName {
  static constexpr std::string_view value = "value";
};

/** @brief Metavar name for `std::string`: `"string"`. */
template <>
struct MetavarName<std::string> {
  static constexpr std::string_view value = "string";
};

/** @brief Metavar name for `bool`: `"bool"`. */
template <>
struct MetavarName<bool> {
  static constexpr std::string_view value = "bool";
};

/** @brief Metavar name for `float`: `"float"`. */
template <>
struct MetavarName<float> {
  static constexpr std::string_view value = "float";
};

/** @brief Metavar name for `double`: `"double"`. */
template <>
struct MetavarName<double> {
  static constexpr std::string_view value = "double";
};

/** @brief Metavar name for `std::filesystem::path`: `"path"`. */
template <>
struct MetavarName<std::filesystem::path> {
  static constexpr std::string_view value = "path";
};

/** @brief Metavar name for signed integer types: `"int"`. */
template <std::signed_integral T>
struct MetavarName<T> {
  static constexpr std::string_view value = "int";
};

/** @brief Metavar name for unsigned integer types: `"uint"`. */
template <std::unsigned_integral T>
struct MetavarName<T> {
  static constexpr std::string_view value = "uint";
};

/**
 * @brief Helper that unwraps storage wrapper types to their underlying value
 * type.
 *
 * - `UnwrapStorage<T>` → `type = T`, `variadic = false`
 * - `UnwrapStorage<std::optional<T>>` → `type = T`, `variadic = false`
 * - `UnwrapStorage<std::vector<T>>` → `type = T`, `variadic = true`
 *
 * @tparam T The potentially-wrapped storage type.
 */
template <class T>
struct UnwrapStorage {
  using type = std::remove_cvref_t<T>;
  static constexpr bool variadic = false;
};

/** @brief Specialization for `std::optional<T>`. */
template <class T>
struct UnwrapStorage<std::optional<T>> {
  using type = std::remove_cvref_t<T>;
  static constexpr bool variadic = false;
};

/** @brief Specialization for `std::vector<T>` (variadic storage). */
template <class T>
struct UnwrapStorage<std::vector<T>> {
  using type = std::remove_cvref_t<T>;
  static constexpr bool variadic = true;
};

/** @brief Alias for the underlying value type of a storage wrapper. */
template <class T>
using unwrap_storage_t = typename UnwrapStorage<std::remove_cvref_t<T>>::type;

/**
 * @brief Returns the metavar string for a storage type, e.g. `<int>` or
 * `<path...>`.
 *
 * For variadic storage (i.e. `std::vector<T>`), `...` is inserted before the
 * closing `>`, producing e.g. `<string...>`.
 *
 * @tparam T The storage type of the argument field.
 * @return A string such as `"<string>"` or `"<path...>"`.
 */
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

/**
 * @brief Builds the usage-line suffix that summarises what `value` accepts.
 *
 * Iterates over all fields of `value` and appends `" [options]"`,
 * `" [args]"`, and/or `" [command]"` as appropriate.
 *
 * @tparam T The argument specification type.
 * @param value An instance of the argument specification.
 * @return A suffix string such as `" [options] [args]"`.
 */
template <class T>
[[nodiscard]]
auto command_usage_suffix(T& value) -> std::string {
  std::string out;
  bool has_options = false;
  bool has_positionals = false;
  bool has_commands = false;

  std::apply(
      [&](auto&... fields) -> auto {
        (
            [&]() -> auto {
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

/**
 * @brief Returns the label used in the help table for a single field type.
 *
 * - For `OptionTag` fields: `"-s, --long <metavar>"` (short name omitted if
 * `'\0'`).
 * - For `PositionalTag` fields: `"<metavar>"`.
 * - For `CommandTag` fields: the command name string.
 * - Otherwise: an empty string.
 *
 * @tparam T The field type (derived from one of the tag base classes).
 * @return The formatted label string.
 */
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
    return std::string(F::command_name());
  } else {
    return std::string{};
  }
}

/**
 * @brief Splits a string on newline characters.
 *
 * Always returns at least one element. If `text` is empty the result
 * contains one empty string.
 *
 * @param text The string to split.
 * @return A vector of lines (without the newline characters).
 */
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

/**
 * @brief Appends a description string to `out`, indenting continuation lines.
 *
 * If `description` is empty a bare newline is appended. Otherwise the first
 * line is written as-is, and each subsequent line is prefixed with `indent`
 * spaces so that it aligns with the first line in the help table.
 *
 * @param out         The output string to append to.
 * @param description The description text, potentially containing newlines.
 * @param indent      Number of spaces to prepend to continuation lines.
 */
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

/**
 * @brief Appends a labelled section (Options, Positional arguments, Commands) to
 * `out`.
 *
 * Iterates over all fields of `value`, selects those for which `pred`
 * returns `true`, renders each via `render`, and emits a right-aligned
 * table under the section `title`.
 *
 * @tparam T      The argument specification type.
 * @tparam Pred   A consteval callable `template<class F>() -> bool`.
 * @tparam Render A callable `(field, label) -> std::string`.
 * @param out           The output string to append to.
 * @param title         Section heading text (e.g. `"Options:"`).
 * @param heading_color ANSI sequence for the section heading (may be empty).
 * @param reset_color   ANSI reset sequence (may be empty).
 * @param value         The argument specification instance.
 * @param pred          Field filter predicate.
 * @param render        Row renderer called for each selected field.
 */
template <class T, class Pred, class Render>
auto append_section(std::string& out, std::string_view title,
                    std::string_view heading_color, std::string_view reset_color,
                    T& value, Pred pred, Render render) -> void {
  std::vector<std::string> rows;
  std::size_t width = 0;

  std::apply(
      [&](auto&... fields) -> auto {
        (
            [&]() -> auto {
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

/**
 * @brief Renders a single row in the help table for a field.
 *
 * Produces a string of the form:
 * @code
 *   "  <label>  <description>\n"
 * @endcode
 * The label is left-padded to `width` characters so all descriptions are
 * aligned. Multi-line descriptions are indented to match.
 *
 * @tparam Field The field type (must have a `.help` member).
 * @param field The field instance.
 * @param label The pre-computed label string for this field.
 * @param width The maximum label width in the current section (for alignment).
 * @return The formatted row string.
 */
template <class Field>
auto render_help_row(Field& field, std::string_view label, std::size_t width)
    -> std::string {
  std::string out = "  ";
  out += label;
  if (!field.help_text().empty()) {
    out.append(width - label.size() + 2, ' ');
    append_wrapped_description(out, field.help_text(), width + 4);
  } else {
    out += '\n';
  }
  return out;
}

/**
 * @brief Extracts the `Description` field's text from an argument specification
 * instance.
 *
 * Iterates over all fields; if a field derives from `DescriptionTag` its
 * string value is returned. Returns an empty view if no description is found.
 *
 * @tparam T The argument specification type.
 * @param value An instance of the argument specification.
 * @return A `string_view` into the description string, or an empty view.
 */
template <class T>
auto find_description(T& value) -> std::string_view {
  std::string_view description;
  std::apply(
      [&](auto&... fields) -> auto {
        (
            [&]() -> auto {
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

/**
 * @brief Core implementation of help text generation.
 *
 * Builds the full help string including:
 * - A `"Usage: <program> [options] [args] [command]"` line.
 * - An optional description paragraph.
 * - An `"Options:"` section for all `OptionTag` fields.
 * - A `"Positional arguments:"` section for all `PositionalTag` fields.
 * - A `"Commands:"` section for all `CommandTag` fields.
 * - Optionally, recursively appended help for each subcommand.
 *
 * Labels within each section are right-aligned to the widest entry.
 *
 * @tparam T The argument specification type.
 * @param value        An instance of the argument specification.
 * @param program_name The program name shown in the usage line.
 * @param color_mode   Controls ANSI color output.
 * @param recurse      If `true`, recursively append help for subcommands.
 * @param command_path The subcommand path prefix appended after `program_name`
 * (empty for root).
 * @return The formatted help string.
 */
template <class T>
auto format_help_impl(T& value, std::string_view program_name,
                      ColorMode color_mode, bool recurse,
                      std::string_view command_path) -> std::string {
  const auto style = resolve_color(color_mode);
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
      [&](auto&... fields) -> auto {
        (
            [&]() -> auto {
              using F = std::remove_cvref_t<decltype(fields)>;
              if constexpr (std::derived_from<F, OptionTag>) {
                auto label = field_usage_label<F>();
                option_width = std::max(option_width, label.size());
                option_rows.emplace_back(std::move(label), fields.help_text());
              } else if constexpr (std::derived_from<F, PositionalTag>) {
                auto label = field_usage_label<F>();
                positional_width = std::max(positional_width, label.size());
                positional_rows.emplace_back(std::move(label),
                                             fields.help_text());
              } else if constexpr (std::derived_from<F, CommandTag>) {
                auto label = field_usage_label<F>();
                command_width = std::max(command_width, label.size());
                command_rows.emplace_back(std::move(label), fields.help_text());
              }
            }(),
            ...);
      },
      as_tuple(value));

  auto append_rows =
      [&](std::string_view title,
          const std::vector<std::pair<std::string, std::string_view>>& rows,
          std::size_t width) -> auto {
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
        [&](auto&... fields) -> auto {
          (
              [&]() -> auto {
                using F = std::remove_cvref_t<decltype(fields)>;
                if constexpr (std::derived_from<F, CommandTag>) {
                  out += '\n';
                  out += format_help_impl<typename F::argument_type>(
                      static_cast<typename F::argument_type&>(fields),
                      program_name, color_mode, true, F::command_name());
                }
              }(),
              ...);
        },
        as_tuple(value));
  }

  return out;
}

}  // namespace detail

/**
 * @brief Generates a complete help string for an argument specification.
 *
 * This is the primary public API for help text generation. It delegates to
 * `detail::format_help_impl` with an empty `command_path`.
 *
 * @tparam T The argument specification type (must satisfy `ArgumentSpec`).
 * @param value        An instance of the argument specification.
 * @param program_name The program name to display in the usage line. Default:
 * `"program"`.
 * @param color_mode   Controls ANSI color output. Default: `ColorMode::auto_`.
 * @param recurse      If `true`, also append help for each subcommand. Default:
 * `false`.
 * @return The formatted help string.
 */
template <ArgumentSpec T>
auto format_help(T& value, std::string_view program_name = "program",
                 ColorMode color_mode = ColorMode::auto_, bool recurse = false)
    -> std::string {
  return detail::format_help_impl<T>(value, program_name, color_mode, recurse,
                                     {});
}

}  // namespace cli
