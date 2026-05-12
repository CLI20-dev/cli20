#pragma once

#include <algorithm>
#include <iterator>
#include <optional>
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
  std::string out = "<";
  out += MetavarName<Base>::value;
  out += ">";
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
                if (!fields.hidden()) has_options = true;
              } else if constexpr (std::derived_from<F, PositionalTag>) {
                if (!fields.hidden()) has_positionals = true;
              } else if constexpr (std::derived_from<F, CommandTag>) {
                if (!fields.hidden()) has_commands = true;
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
      out += "-";
      out += F::short_name;
      out += ", ";
    }
    out += "--";
    out += F::name.view();
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

template <class T>
[[nodiscard]]
auto styled_field_usage_label(const AnsiStyle& style) -> std::string {
  using F = std::remove_cvref_t<T>;
  const auto reset = style.reset();
  if constexpr (std::derived_from<F, OptionTag>) {
    std::string out;
    if constexpr (F::short_name != '\0') {
      out += style.option();
      out += "-";
      out += F::short_name;
      out += reset;
      out += ", ";
    }
    out += style.option();
    out += "--";
    out += F::name.view();
    out += reset;
    if constexpr (!(F::nargs.min == 0 && F::nargs.max == 0)) {
      out += " ";
      out += style.metavar();
      out += metavar_for<typename F::value_type>();
      out += reset;
    }
    return out;
  } else if constexpr (std::derived_from<F, PositionalTag>) {
    std::string out = std::string(style.metavar());
    out += metavar_for<typename F::value_type>();
    out += reset;
    return out;
  } else if constexpr (std::derived_from<F, CommandTag>) {
    std::string out = std::string(style.command());
    out += F::command_name();
    out += reset;
    return out;
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
consteval auto relation_field_name() -> std::string_view {
  using F = std::remove_cvref_t<Field>;
  if constexpr (std::derived_from<F, OptionTag> ||
                std::derived_from<F, CommandTag>) {
    return F::name.view();
  } else {
    return {};
  }
}

inline auto relation_name_list(NameList<> names) -> std::string {
  std::string out;
  for (std::size_t i = 0; i < names.size; ++i) {
    if (i != 0) out += ", ";
    out += "--";
    out += names.values[i];
  }
  return out;
}

template <class T>
constexpr auto relation_group(std::string_view name) -> NameList<> {
  NameList<> result;
  if constexpr (detail::HasRelations<T>) {
    std::apply(
        [&](auto... rels) -> auto {
          (
              [&]() -> auto {
                using R = std::remove_cvref_t<decltype(rels)>;
                if constexpr (std::same_as<R, GroupRelation>) {
                  if (rels.name == name) result = rels.members;
                }
              }(),
              ...);
        },
        T::relations.items);
  }
  return result;
}

template <class T>
constexpr auto relation_operand_contains(std::string_view operand,
                                         std::string_view field_name) -> bool {
  if (operand == field_name) return true;
  const auto group = relation_group<T>(operand);
  for (auto member : group) {
    if (member == field_name) return true;
  }
  return false;
}

template <class T>
auto relation_operand_label(std::string_view operand) -> std::string {
  if constexpr (detail::HasRelations<T>) {
    if (!relation_group<T>(operand).empty()) {
      return std::string(operand);
    }
  }
  return "--" + std::string(operand);
}

template <class T, class Field>
auto relation_help_metadata() -> std::vector<std::string> {
  std::vector<std::string> metadata;
  constexpr auto field_name = relation_field_name<Field>();
  if constexpr (field_name.empty()) {
    return metadata;
  } else if constexpr (detail::HasRelations<T>) {
    std::apply(
        [&](auto... rels) -> auto {
          (
              [&]() -> auto {
                using R = std::remove_cvref_t<decltype(rels)>;
                if constexpr (std::same_as<R, GroupRelation>) {
                  (void)field_name;
                } else if constexpr (std::same_as<R, ConflictsRelation>) {
                  if (relation_operand_contains<T>(rels.left, field_name)) {
                    metadata.emplace_back("conflicts: " +
                                          relation_operand_label<T>(rels.right));
                  }
                  if (relation_operand_contains<T>(rels.right, field_name)) {
                    metadata.emplace_back("conflicts: " +
                                          relation_operand_label<T>(rels.left));
                  }
                } else if constexpr (std::same_as<R, DependsOnRelation>) {
                  if (relation_operand_contains<T>(rels.source, field_name)) {
                    metadata.emplace_back(
                        "requires: " + relation_operand_label<T>(rels.target));
                  }
                  if (relation_operand_contains<T>(rels.target, field_name)) {
                    metadata.emplace_back(
                        "required by: " +
                        relation_operand_label<T>(rels.source));
                  }
                }
              }(),
              ...);
        },
        T::relations.items);
    return metadata;
  } else {
    return metadata;
  }
}

template <class T, class Field>
auto help_metadata(Field& field) -> std::vector<std::string> {
  std::vector<std::string> metadata;
  if constexpr (std::derived_from<std::remove_cvref_t<Field>, OptionTag> ||
                std::derived_from<std::remove_cvref_t<Field>, PositionalTag>) {
    if (field.presence() == Presence::required) {
      metadata.emplace_back("required");
    }
    if (!field.env().empty()) {
      metadata.emplace_back("env: " + std::string(field.env()));
    }
    if (const auto& default_value = field.default_value()) {
      using Value = std::remove_cvref_t<decltype(*default_value)>;
      if constexpr (requires(std::ostream& os, const Value& value) {
                      os << value;
                    }) {
        std::ostringstream out;
        out << *default_value;
        metadata.emplace_back("default: " + out.str());
      }
    }
  }
  if (!field.deprecated().empty()) {
    metadata.emplace_back("deprecated: " + std::string(field.deprecated()));
  }
  auto relation_metadata = relation_help_metadata<T, Field>();
  metadata.insert(metadata.end(),
                  std::make_move_iterator(relation_metadata.begin()),
                  std::make_move_iterator(relation_metadata.end()));
  return metadata;
}

inline auto append_help_metadata(std::string& out,
                                 const std::vector<std::string>& metadata,
                                 const AnsiStyle& style) -> void {
  if (metadata.empty()) {
    return;
  }
  out += style.metadata();
  out += " [";
  for (std::size_t i = 0; i < metadata.size(); ++i) {
    if (i != 0) {
      out += "] [";
    }
    out += metadata[i];
  }
  out += "]";
  out += style.reset();
}

inline auto append_row_description(std::string& out,
                                   std::string_view description,
                                   const std::vector<std::string>& metadata,
                                   const AnsiStyle& style, std::size_t indent)
    -> void {
  auto lines = split_lines(description);
  out += lines.front();
  append_help_metadata(out, metadata, style);
  out += '\n';
  for (std::size_t i = 1; i < lines.size(); ++i) {
    out.append(indent, ' ');
    out += lines[i];
    out += '\n';
  }
}

template <class T>
auto append_group_requirements(std::string& row, std::string_view group_name)
    -> void {
  if constexpr (detail::HasRelations<T>) {
    std::apply(
        [&](auto... rels) -> auto {
          (
              [&]() -> auto {
                using R = std::remove_cvref_t<decltype(rels)>;
                if constexpr (std::same_as<R, DependsOnRelation>) {
                  if (rels.source == group_name) {
                    row += "    requires: ";
                    row += relation_operand_label<T>(rels.target);
                    row += "\n";
                  }
                }
              }(),
              ...);
        },
        T::relations.items);
  }
}

template <class T>
auto relation_option_group_row(const GroupRelation& group,
                               const AnsiStyle& style) -> std::string {
  std::string row = "  ";
  row += style.group();
  row += group.name;
  row += style.reset();
  row += ":\n    ";
  row += relation_name_list(group.members);
  row += " must be used together\n";
  append_group_requirements<T>(row, group.name);
  return row;
}

template <class T>
auto relation_option_group_rows(const AnsiStyle& style)
    -> std::vector<std::string> {
  std::vector<std::string> rows;
  if constexpr (detail::HasRelations<T>) {
    std::apply(
        [&](auto... rels) -> auto {
          (
              [&]() -> auto {
                using R = std::remove_cvref_t<decltype(rels)>;
                if constexpr (std::same_as<R, GroupRelation>) {
                  rows.push_back(relation_option_group_row<T>(rels, style));
                }
              }(),
              ...);
        },
        T::relations.items);
  }
  return rows;
}

template <class T, class Field>
auto render_help_row(Field& field, std::string_view label,
                     std::string_view styled_label, std::size_t width,
                     const AnsiStyle& style) -> std::string {
  std::string out = "  ";
  out += styled_label;
  const auto metadata = help_metadata<T>(field);
  if (!field.help_text().empty() || !metadata.empty()) {
    out.append(width - label.size() + 2, ' ');
    append_row_description(out, field.help_text(), metadata, style, width + 4);
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
                      std::string_view command_path,
                      HelpPalette palette = default_help_palette)
    -> std::string {
  const auto style = resolve_color(color_mode, palette);
  const std::string heading = std::string(style.heading());
  const std::string reset = std::string(style.reset());

  std::string full_program = std::string(program_name);
  if (!command_path.empty()) {
    full_program += " ";
    full_program += command_path;
  }

  std::string out = std::string(style.usage());
  out += "Usage:";
  out += reset;
  out += " ";
  out += full_program;
  out += command_usage_suffix(value);
  out += "\n";

  if (const auto description = find_description(value); !description.empty()) {
    out += '\n';
    out += description;
    out += '\n';
  }

  std::vector<std::pair<std::string, std::string>> option_rows;
  std::vector<std::pair<std::string, std::string>> positional_rows;
  std::vector<std::pair<std::string, std::string>> command_rows;
  std::size_t option_width = 0;
  std::size_t positional_width = 0;
  std::size_t command_width = 0;

  std::apply(
      [&](auto&... fields) -> auto {
        (
            [&]() -> auto {
              using F = std::remove_cvref_t<decltype(fields)>;
              if constexpr (std::derived_from<F, OptionTag>) {
                if (fields.hidden()) return;
                auto label = field_usage_label<F>();
                option_width = std::max(option_width, label.size());
                option_rows.emplace_back(std::move(label), std::string{});
              } else if constexpr (std::derived_from<F, PositionalTag>) {
                if (fields.hidden()) return;
                auto label = field_usage_label<F>();
                positional_width = std::max(positional_width, label.size());
                positional_rows.emplace_back(std::move(label), std::string{});
              } else if constexpr (std::derived_from<F, CommandTag>) {
                if (fields.hidden()) return;
                auto label = field_usage_label<F>();
                command_width = std::max(command_width, label.size());
                command_rows.emplace_back(std::move(label), std::string{});
              }
            }(),
            ...);
      },
      as_tuple(value));

  auto append_rows =
      [&](std::string_view title,
          const std::vector<std::pair<std::string, std::string>>& rows) -> auto {
    if (rows.empty()) {
      return;
    }
    out += '\n';
    out += heading;
    out += title;
    out += reset;
    out += '\n';
    for (const auto& [label, row] : rows) {
      (void)label;
      out += row;
    }
  };

  option_rows.clear();
  positional_rows.clear();
  command_rows.clear();
  std::apply(
      [&](auto&... fields) -> auto {
        (
            [&]() -> auto {
              using F = std::remove_cvref_t<decltype(fields)>;
              if constexpr (std::derived_from<F, OptionTag>) {
                if (fields.hidden()) return;
                auto label = field_usage_label<F>();
                auto styled_label = styled_field_usage_label<F>(style);
                option_rows.emplace_back(
                    label, render_help_row<T>(fields, label, styled_label,
                                              option_width, style));
              } else if constexpr (std::derived_from<F, PositionalTag>) {
                if (fields.hidden()) return;
                auto label = field_usage_label<F>();
                auto styled_label = styled_field_usage_label<F>(style);
                positional_rows.emplace_back(
                    label, render_help_row<T>(fields, label, styled_label,
                                              positional_width, style));
              } else if constexpr (std::derived_from<F, CommandTag>) {
                if (fields.hidden()) return;
                auto label = field_usage_label<F>();
                auto styled_label = styled_field_usage_label<F>(style);
                command_rows.emplace_back(
                    label, render_help_row<T>(fields, label, styled_label,
                                              command_width, style));
              }
            }(),
            ...);
      },
      as_tuple(value));

  append_rows("Options:", option_rows);
  append_rows("Positional arguments:", positional_rows);
  append_rows("Commands:", command_rows);

  const auto option_group_rows = relation_option_group_rows<T>(style);
  if (!option_group_rows.empty()) {
    out += '\n';
    out += heading;
    out += "Option groups:";
    out += reset;
    out += '\n';
    for (const auto& row : option_group_rows) {
      out += row;
    }
  }

  if (recurse && !command_rows.empty()) {
    std::apply(
        [&](auto&... fields) -> auto {
          (
              [&]() -> auto {
                using F = std::remove_cvref_t<decltype(fields)>;
                if constexpr (std::derived_from<F, CommandTag>) {
                  if (fields.hidden()) return;
                  out += '\n';
                  out += format_help_impl<typename F::argument_type>(
                      static_cast<typename F::argument_type&>(fields),
                      program_name, color_mode, true, F::command_name(),
                      palette);
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
 * @param color_mode   Controls ANSI color output. Default: `ColorMode::detect`.
 * @param recurse      If `true`, also append help for each subcommand. Default:
 * `false`.
 * @return The formatted help string.
 */
template <ArgumentSpec T>
auto format_help(T& value, std::string_view program_name = "program",
                 ColorMode color_mode = ColorMode::detect, bool recurse = false,
                 HelpPalette palette = default_help_palette) -> std::string {
  return detail::format_help_impl<T>(value, program_name, color_mode, recurse,
                                     {}, palette);
}

}  // namespace cli
