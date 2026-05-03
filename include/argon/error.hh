#pragma once

#include <string>
namespace argon {

enum class ErrorCode {
  UnknownOption,
  MissingValue,
  InvalidValue,
  UnknownError,
};

struct ParseError {
  std::string message;
  ErrorCode code = ErrorCode::UnknownError;
  int position = -1;
  explicit ParseError(std::string message, ErrorCode code = ErrorCode::UnknownError,
                      int position = -1)
      : message(std::move(message)), code(code), position(position) {}
};

}  // namespace argon
