find_package(GTest REQUIRED)
include(GoogleTest)

file(GLOB TEST_SOURCES "*.cc")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(cxx_template_tests ${TEST_SOURCES})

target_link_libraries(
  cxx_template_tests PRIVATE cxx_template GTest::gtest GTest::gtest_main
)

target_compile_features(cxx_template_tests PRIVATE cxx_std_26)
target_compile_options(cxx_template_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)

gtest_discover_tests(cxx_template_tests)
