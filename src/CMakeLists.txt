# glob
file(GLOB_RECURSE SOURCES "*.cc")

add_library(argon ${SOURCES})

target_include_directories(
  argon
  PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
)

target_compile_features(argon PUBLIC cxx_std_26)
target_compile_options(argon PRIVATE -Wall -Wextra -Wpedantic -Werror)

install(TARGETS argon LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include)
