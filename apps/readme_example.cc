#include <iostream>

#include "argon/arithmetic_argument.hh"
#include "argon/flag_argument.hh"
#include "argon/parser.hh"
#include "argon/string_argument.hh"

struct Args {
    argon::FlagArg<"help",    'h'> help   {"Show this help message"};
    argon::FlagArg<"verbose", 'v'> verbose{"Enable verbose output"};
    argon::IntArg<"count",    'n'> count  {"Number of iterations"};
    argon::StrArg<"output",   'o'> output {"Output file path"};
};

auto main(int argc, char** argv) -> int {
    argon::Parser<Args> parser;
    auto res = parser.parse(argc, argv);

    if (!res) {
        std::cerr << "error: " << res.error().message() << '\n';
        std::cerr << parser.formatHelp();
        return 1;
    }
    if (res->help.provided()) {
        std::cout << parser.formatHelp();
        return 0;
    }
    if (res->verbose.provided()) { /* ... */ }
    if (res->count.provided())   { int _ = res->count.value(); }
}
