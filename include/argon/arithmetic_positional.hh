#pragma once

#include <argon/argument.hh>
#include <cstdint>

namespace argon {

// ---- Aliases ----
// PositionalArgument<T> for arithmetic T is handled by the primary template
// in argument.hh, which provides a constrained parse() member for ArithmeticParseable types.

using IntPositional    = PositionalArgument<int>;
using Int32Positional  = PositionalArgument<int32_t>;
using Int64Positional  = PositionalArgument<int64_t>;
using Uint32Positional = PositionalArgument<uint32_t>;
using Uint64Positional = PositionalArgument<uint64_t>;
using FloatPositional  = PositionalArgument<float>;
using DoublePositional = PositionalArgument<double>;

}  // namespace argon
