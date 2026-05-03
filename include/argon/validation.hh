#pragma once

#include <string>

namespace argon {

enum class ArithmeticValidationRule {
  none,
  positive,
  non_negative,
  negative,
  non_positive,
};

}
