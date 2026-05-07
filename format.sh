#!/usr/bin/env sh
find include src tests apps modules -name '*.hh' -o -name '*.cc' | xargs clang-format -i
