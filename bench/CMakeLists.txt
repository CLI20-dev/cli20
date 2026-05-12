cmake_minimum_required(VERSION 3.20)

project(cli20_bench LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/BenchLibraries.cmake)

add_subdirectory(runtime)
add_subdirectory(size)
