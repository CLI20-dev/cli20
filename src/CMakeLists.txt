add_library(cxx_template template.cc)

target_include_directories(
  cxx_template
  PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
)

target_compile_features(cxx_template PUBLIC cxx_std_26)
target_compile_options(cxx_template PRIVATE -Wall -Wextra -Wpedantic -Werror)

install(TARGETS cxx_template LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include)
