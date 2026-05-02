#pragma once

namespace argon {

#define ARGON_ENABLE_COMPILETIME_VALIDATION

template <bool T>
#ifdef ARGON_ENABLE_COMPILETIME_VALIDATION
  requires(T)
#endif
struct Validate {
};

struct Success : Validate<true> {};

// struct Failure : Validate<false> {};

}  // namespace argon
