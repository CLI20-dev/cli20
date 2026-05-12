include_guard(GLOBAL)

get_filename_component(CLI20_REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

add_library(bench_common INTERFACE)
target_include_directories(bench_common INTERFACE
  "${CLI20_REPOSITORY_ROOT}/include"
  "${CLI20_REPOSITORY_ROOT}/bench/include"
)
target_compile_features(bench_common INTERFACE cxx_std_20)

add_library(bench_warnings INTERFACE)
target_compile_options(bench_warnings INTERFACE
  $<IF:$<CXX_COMPILER_ID:MSVC>,/W4,-Wall;-Wextra;-Wpedantic>
)

find_package(benchmark REQUIRED)
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
find_package(Boost REQUIRED COMPONENTS program_options)

find_path(BENCH_CLI11_INCLUDE_DIR CLI/CLI.hpp REQUIRED)
find_path(BENCH_ARGPARSE_INCLUDE_DIR argparse/argparse.hpp REQUIRED)
find_path(BENCH_CXXOPTS_INCLUDE_DIR cxxopts.hpp REQUIRED)

add_library(bench_cli11 INTERFACE)
target_include_directories(bench_cli11 INTERFACE "${BENCH_CLI11_INCLUDE_DIR}")

add_library(bench_argparse INTERFACE)
target_include_directories(bench_argparse INTERFACE "${BENCH_ARGPARSE_INCLUDE_DIR}")

add_library(bench_cxxopts INTERFACE)
target_include_directories(bench_cxxopts INTERFACE "${BENCH_CXXOPTS_INCLUDE_DIR}")

add_library(bench_boost_program_options INTERFACE)
target_link_libraries(bench_boost_program_options INTERFACE Boost::program_options)

function(bench_add_size_executable target source library_target)
  add_executable("${target}" "${source}")
  target_link_libraries("${target}" PRIVATE
    bench_common
    bench_warnings
    "${library_target}"
  )
endfunction()
