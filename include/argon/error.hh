#pragma once

#include <string>

namespace argon {

enum class ErrorCode {
  unknown_option,
  unknown_command,
  unexpected_argument,
  missing_value,
  invalid_value,
  duplicate_argument,

  conversion_error,
  out_of_range,
  missing_required,
  mutually_exclusive,
  dependency_missing,
  invalid_choice,
  validation_failed,
  unknown_error,
};

enum class ErrorKind {
  parse,
  conversion,
  validation,
};

[[nodiscard]] constexpr auto toString(ErrorCode code) noexcept -> std::string_view {
  switch (code) {
    case ErrorCode::unknown_option:
      return "unknown option";
    case ErrorCode::unknown_command:
      return "unknown command";
    case ErrorCode::unexpected_argument:
      return "unexpected argument";
    case ErrorCode::missing_value:
      return "missing value";
    case ErrorCode::invalid_value:
      return "invalid value";
    case ErrorCode::duplicate_argument:
      return "duplicate argument";
    case ErrorCode::conversion_error:
      return "conversion error";
    case ErrorCode::out_of_range:
      return "out of range";
    case ErrorCode::missing_required:
      return "missing required argument";
    case ErrorCode::mutually_exclusive:
      return "mutually exclusive argument";
    case ErrorCode::dependency_missing:
      return "dependency missing";
    case ErrorCode::invalid_choice:
      return "invalid choice";
    case ErrorCode::validation_failed:
      return "validation failed";
    case ErrorCode::unknown_error:
      return "unknown error";
  }

  return "unknown error";
}

struct ParseError {
  ErrorCode code = ErrorCode::unknown_error;
  ErrorKind kind = ErrorKind::parse;

  int position = -1;

  std::string subject{};
  std::string detail{};

  [[nodiscard]] constexpr auto hasPosition() const noexcept -> bool { return position >= 0; }

  [[nodiscard]] auto message() const -> std::string {
    std::string out{toString(code)};
    if (!subject.empty()) {
      out += ": ";
      out += subject;
    }

    if (!detail.empty()) {
      out += " (";
      out += detail;
      out += ")";
    }

    if (hasPosition()) {
      out += " at argv[";
      out += std::to_string(position);
      out += "]";
    }

    return out;
  }

  [[nodiscard]] auto hasError() const noexcept -> bool { return code != ErrorCode::unknown_error; }
};

}  // namespace argon
