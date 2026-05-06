function(add_cli20_app app_source suffix cxx_std)
  get_filename_component(app_name ${app_source} NAME_WE)
  set(target_name ${app_name}${suffix})

  add_executable(${target_name} ${app_source})
  set_target_properties(${target_name} PROPERTIES CXX_CLANG_TIDY "")

  if(app_name STREQUAL "example_single_header")
    target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/single_header)
  else()
    target_link_libraries(${target_name} PRIVATE cli20)
  endif()

  target_compile_features(${target_name} PUBLIC ${cxx_std})
  target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endfunction()

file(GLOB APP_SOURCES CONFIGURE_DEPENDS "*.cc")

foreach(app_source ${APP_SOURCES})
  add_cli20_app(${app_source} "" cxx_std_20)
  add_cli20_app(${app_source} "_cxx26" cxx_std_26)
endforeach()
