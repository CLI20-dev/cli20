cmake_minimum_required(VERSION 3.20)

project(cli20_bench LANGUAGES CXX)

set(BENCH_CXX_STANDARD "20" CACHE STRING "C++ standard used for benchmark builds")
set_property(CACHE BENCH_CXX_STANDARD PROPERTY STRINGS 20 26)

set(CMAKE_CXX_STANDARD "${BENCH_CXX_STANDARD}")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/BenchLibraries.cmake)

add_subdirectory(runtime)
add_subdirectory(size)
