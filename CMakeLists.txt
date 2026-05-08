cmake_minimum_required(VERSION 3.20)

project(cli20 VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CXX_CLI20_ENABLE_TEST "Enable building tests" OFF)
option(CXX_CLI20_ENABLE_MODULE "Enable the CLI20 module" OFF)

option(CXX_CLI20_ENABLE_CLANG_TIDY "Enable clang-tidy checks" OFF)
if(CXX_CLI20_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE REQUIRED NAMES "clang-tidy")
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
endif()

option(CXX_CLI20_ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
if(CXX_CLI20_ENABLE_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

file(GLOB_RECURSE CLI20_PUBLIC_HEADERS CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh"
)

add_library(cli20 INTERFACE)
add_library(cli20::cli20 ALIAS cli20)

target_sources(cli20
    INTERFACE
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES ${CLI20_PUBLIC_HEADERS}
)

target_include_directories(cli20
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(cli20 INTERFACE cxx_std_20)

set(CLI20_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/cli20")
set(CLI20_TARGETS_EXPORT_NAME cli20Targets)

install(
  TARGETS cli20
  EXPORT ${CLI20_TARGETS_EXPORT_NAME}
  FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  EXPORT ${CLI20_TARGETS_EXPORT_NAME}
  FILE ${CLI20_TARGETS_EXPORT_NAME}.cmake
  NAMESPACE cli20::
  DESTINATION ${CLI20_INSTALL_CMAKEDIR}
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cli20Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cli20Config.cmake
  INSTALL_DESTINATION ${CLI20_INSTALL_CMAKEDIR}
  NO_SET_AND_CHECK_MACRO
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/cli20ConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/cli20Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cli20ConfigVersion.cmake
  DESTINATION ${CLI20_INSTALL_CMAKEDIR}
)


find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  file(GLOB CLI20_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/cli/*.hh"
  )
  set(CLI20_SINGLE_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/single_header/cli20.hh")
  add_custom_command(
    OUTPUT "${CLI20_SINGLE_HEADER}"
    COMMAND Python3::Interpreter script/make_single_header.py
    DEPENDS ${CLI20_HEADERS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating single_header/cli20.hh"
  )
  add_custom_target(cli20_single_header DEPENDS "${CLI20_SINGLE_HEADER}")
  install(
    FILES "${CLI20_SINGLE_HEADER}"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
else()
  message(STATUS "Python3 not found – single_header target and example are disabled")
endif()

add_subdirectory(examples)

if(CXX_CLI20_ENABLE_TEST)
  enable_testing()
  add_subdirectory(tests)
endif()

if(CXX_CLI20_ENABLE_MODULE)
  add_subdirectory(modules)
endif()
