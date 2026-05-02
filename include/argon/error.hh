#pragma once

#include <string>
namespace argon {

enum class ErrorCode {
  UnknownOption,
  MissingValue,
  InvalidValue,
};

struct Error {
  std::string message;
  explicit Error(std::string message) : message(std::move(message)) {}
};

}  // namespace argon
