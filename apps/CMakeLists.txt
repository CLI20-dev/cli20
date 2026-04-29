file(GLOB APP_SOURCES CONFIGURE_DEPENDS "*.cc")

foreach(app_source ${APP_SOURCES})
  get_filename_component(app_name ${app_source} NAME_WE)
  add_executable(${app_name} ${app_source})
  target_link_libraries(${app_name} PRIVATE argon)
  target_compile_features(${app_name} PRIVATE cxx_std_26)
  target_compile_options(${app_name} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endforeach()
