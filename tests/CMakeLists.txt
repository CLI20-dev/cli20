find_package(GTest REQUIRED)
include(GoogleTest)

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "*.cc")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}")

function(add_argon_tests suffix cxx_std)
  set(target_name argon_tests${suffix})
  add_executable(${target_name} ${TEST_SOURCES})
  target_link_libraries(${target_name} PRIVATE argon GTest::gtest GTest::gtest_main)
  target_compile_features(${target_name} PRIVATE ${cxx_std})
  target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  gtest_discover_tests(${target_name})
endfunction()

add_argon_tests("_cxx20" cxx_std_20)
add_argon_tests("_cxx26" cxx_std_26)

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

