find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(cxx_template_tests test_template.cc)

target_link_libraries(
  cxx_template_tests PRIVATE cxx_template GTest::gtest GTest::gtest_main
)

target_compile_features(cxx_template_tests PRIVATE cxx_std_26)
target_compile_options(cxx_template_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)

gtest_discover_tests(cxx_template_tests)
