function(add_cli20_app app_source suffix cxx_std)
  get_filename_component(app_name ${app_source} NAME_WE)
  set(target_name ${app_name}${suffix})

  add_executable(${target_name} ${app_source})
  set_target_properties(${target_name} PROPERTIES CXX_CLANG_TIDY "")

  if(app_name STREQUAL "example_single_header")
    target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/single_header)
  elseif(app_name STREQUAL "example_module")
    if(CXX_CLI20_ENABLE_MODULE)
      target_link_libraries(${target_name} PRIVATE cli20-module)
      set_target_properties(${target_name} PROPERTIES CXX_SCAN_FOR_MODULES ON)
    else()
      message(FATAL_ERROR "example_module requires CXX_CLI20_ENABLE_MODULE to be ON")
    endif()
  else()
    target_link_libraries(${target_name} PRIVATE cli20)
  endif()

  string(REGEX REPLACE "cxx_std_" "" cxx_std_num ${cxx_std})
  set_target_properties(${target_name} PROPERTIES
    CXX_STANDARD ${cxx_std_num}
    CXX_STANDARD_REQUIRED ON
  )
  target_compile_options(${target_name} PRIVATE
    $<IF:$<CXX_COMPILER_ID:MSVC>,/W4;/WX,-Wall;-Wextra;-Wpedantic;-Werror>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-maybe-uninitialized>
  )
endfunction()

file(GLOB APP_SOURCES CONFIGURE_DEPENDS "*.cc")

# Remove example_module if modules are not enabled
if(NOT CXX_CLI20_ENABLE_MODULE)
  list(REMOVE_ITEM APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/example_module.cc")
endif()

foreach(app_source ${APP_SOURCES})
  add_cli20_app(${app_source} "" cxx_std_20)
  get_filename_component(app_name ${app_source} NAME_WE)
  if(NOT MSVC AND NOT app_name STREQUAL "example_module")
    add_cli20_app(${app_source} "_cxx26" cxx_std_26)
  endif()
endforeach()
