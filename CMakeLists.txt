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

# add_subdirectory(src)

add_library(cli20 INTERFACE)
add_library(cli20::cli20 ALIAS cli20)

target_include_directories(cli20
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(cli20
    INTERFACE
        cxx_std_20
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CLI20_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/cli20")

install(TARGETS cli20 EXPORT cli20-targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  EXPORT cli20-targets
  NAMESPACE cli20::
  DESTINATION ${CLI20_INSTALL_CMAKEDIR}
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cli20Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cli20Config.cmake
  INSTALL_DESTINATION ${CLI20_INSTALL_CMAKEDIR}
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


add_subdirectory(apps)

if(CXX_CLI20_ENABLE_TEST)
  enable_testing()
  add_subdirectory(tests)
endif()

if(CXX_CLI20_ENABLE_MODULE)
  add_subdirectory(modules)
endif()
