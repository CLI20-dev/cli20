cmake_minimum_required(VERSION 3.28)

add_library(cli20-module)
add_library(cli20::module ALIAS cli20-module)

target_sources(cli20-module
  PUBLIC
    FILE_SET CXX_MODULES
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/cli20.cppm
)

set_target_properties(cli20-module
  PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_MODULE_STD 20
    EXPORT_NAME module
)

target_compile_options(cli20-module PRIVATE
  $<IF:$<CXX_COMPILER_ID:MSVC>,/W4;/WX,-Wall;-Wextra;-Wpedantic;-Werror>
  $<$<CXX_COMPILER_ID:GNU>:-Wno-maybe-uninitialized>
)

# Link to the header-only cli20 library for include directories
target_link_libraries(cli20-module PUBLIC cli20)

# Install the module
install(TARGETS cli20-module
  EXPORT ${CLI20_TARGETS_EXPORT_NAME}
  FILE_SET CXX_MODULES DESTINATION ${CLI20_INSTALL_CMAKEDIR}/modules
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
