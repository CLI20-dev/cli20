cmake_minimum_required(VERSION 3.20)

project(Argon LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CXX_ARGON_ENABLE_TEST "Enable building tests" OFF)

option(CXX_ARGON_ENABLE_CLANG_TIDY "Enable clang-tidy checks" OFF)
if(CXX_ARGON_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE REQUIRED NAMES "clang-tidy")
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
endif()

option(CXX_ARGON_ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
if(CXX_ARGON_ENABLE_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

add_subdirectory(src)
add_subdirectory(apps)

if(CXX_ARGON_ENABLE_TEST)
  enable_testing()
  add_subdirectory(tests)
endif()
