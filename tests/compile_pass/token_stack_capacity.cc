#define CLI20_TOKEN_STACK_CAPACITY 4

#include "cli/parser.hh"

static_assert(cli::token_stack_capacity == 4);

auto main() -> int { return 0; }
