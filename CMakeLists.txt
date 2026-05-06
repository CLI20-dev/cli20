cmake_minimum_required(VERSION 3.20)

project(cli20 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CXX_CLI20_ENABLE_TEST "Enable building tests" OFF)

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
install(TARGETS cli20 EXPORT cli20-targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


add_subdirectory(apps)

if(CXX_CLI20_ENABLE_TEST)
  enable_testing()
  add_subdirectory(tests)
endif()
