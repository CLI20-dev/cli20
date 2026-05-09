function(add_cli20_example source_file suffix cxx_std)
  get_filename_component(example_name "${source_file}" NAME_WE)
  set(target_name "${example_name}${suffix}")

  add_executable("${target_name}" "${source_file}")
  set_target_properties("${target_name}" PROPERTIES CXX_CLANG_TIDY "")

  if(example_name STREQUAL "single_header")
    add_dependencies("${target_name}" cli20_single_header)
    cmake_path(GET CLI20_SINGLE_HEADER PARENT_PATH _single_header_dir)
    target_include_directories("${target_name}" PRIVATE "${_single_header_dir}")
  elseif(example_name STREQUAL "module")
    if(CXX_CLI20_ENABLE_MODULE)
      target_link_libraries("${target_name}" PRIVATE cli20-module)
      set_target_properties("${target_name}" PROPERTIES CXX_SCAN_FOR_MODULES ON)
    else()
      message(FATAL_ERROR "module example requires CXX_CLI20_ENABLE_MODULE to be ON")
    endif()
  else()
    target_link_libraries("${target_name}" PRIVATE cli20)
  endif()

  string(REGEX REPLACE "cxx_std_" "" cxx_std_num "${cxx_std}")
  set_target_properties("${target_name}" PROPERTIES
    CXX_STANDARD "${cxx_std_num}"
    CXX_STANDARD_REQUIRED ON
  )
  target_compile_options("${target_name}" PRIVATE
    $<IF:$<CXX_COMPILER_ID:MSVC>,/W4;/WX,-Wall;-Wextra;-Wpedantic;-Werror>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-maybe-uninitialized>
  )
endfunction()

file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS "*.cc")

if(NOT CXX_CLI20_ENABLE_MODULE)
  list(REMOVE_ITEM EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/module.cc")
endif()

foreach(example_source ${EXAMPLE_SOURCES})
  get_filename_component(example_name "${example_source}" NAME_WE)
  if(example_name STREQUAL "single_header" AND NOT TARGET cli20_single_header)
    message(STATUS "Skipping single_header example (cli20_single_header target not available)")
    continue()
  endif()
  add_cli20_example("${example_source}" "" cxx_std_20)
  if(NOT MSVC AND NOT example_name STREQUAL "module")
    add_cli20_example("${example_source}" "_cxx26" cxx_std_26)
  endif()
endforeach()
