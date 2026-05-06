# glob
file(GLOB_RECURSE SOURCES "*.cc")

add_library(cli20_static ${SOURCES})

target_include_directories(
  cli20_static
  PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
)

# target_compile_features(cli20_static PUBLIC cxx_std_20)
target_compile_options(cli20_static PRIVATE -Wall -Wextra -Wpedantic -Werror)

install(TARGETS cli20_static LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include)
