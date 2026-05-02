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

// Long-form *PositionalArg aliases
using IntPositionalArg    = IntPositional;
using Int32PositionalArg  = Int32Positional;
using Int64PositionalArg  = Int64Positional;
using Uint32PositionalArg = Uint32Positional;
using Uint64PositionalArg = Uint64Positional;
using FloatPositionalArg  = FloatPositional;
using DoublePositionalArg = DoublePositional;

// Short-form *PosArg aliases
using IntPosArg    = IntPositional;
using Int32PosArg  = Int32Positional;
using Int64PosArg  = Int64Positional;
using Uint32PosArg = Uint32Positional;
using Uint64PosArg = Uint64Positional;
using FloatPosArg  = FloatPositional;
using DoublePosArg = DoublePositional;

}  // namespace argon
