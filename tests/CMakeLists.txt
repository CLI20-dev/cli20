find_package(GTest REQUIRED)
include(GoogleTest)

file(GLOB TEST_SOURCES "*.cc")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(argon_tests ${TEST_SOURCES})

target_link_libraries(
  argon_tests PRIVATE argon GTest::gtest GTest::gtest_main
)

target_compile_features(argon_tests PRIVATE cxx_std_26)
target_compile_options(argon_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)

gtest_discover_tests(argon_tests)

# Compile-fail tests: verify that duplicate options are rejected at compile time
function(add_compile_fail_test name source)
  try_compile(
    COMPILE_RESULT
    ${CMAKE_BINARY_DIR}/try_compile/${name}
    ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/${source}
    CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${PROJECT_SOURCE_DIR}/include"
    CXX_STANDARD 26
    OUTPUT_VARIABLE COMPILE_OUTPUT
  )
  if(COMPILE_RESULT)
    message(FATAL_ERROR
      "Compile-fail test '${name}' unexpectedly succeeded.\n${COMPILE_OUTPUT}")
  else()
    message(STATUS "Compile-fail test passed: ${name}")
  endif()
endfunction()

add_compile_fail_test(duplicate_long_opt     duplicate_long_opt.cc)
add_compile_fail_test(duplicate_short_opt    duplicate_short_opt.cc)
add_compile_fail_test(duplicate_command_name duplicate_command_name.cc)
add_compile_fail_test(duplicate_in_subcommand duplicate_in_subcommand.cc)
