# Creates the WOPR executable.

find_package(Threads REQUIRED)

set(WOPR_SRC
  command_line.cc
  monitor_data.cc
  results.cc
  run.cc
  wopr.cc
)

if (WOPR_ENABLE_GUI)
  list(APPEND WOPR_SRC
    dashboard.cc
    imgui_app.cc
    monitor.cc
  )
endif()

add_executable(wopr ${WOPR_SRC})

if (WOPR_ENABLE_GUI)
  target_compile_definitions(wopr PRIVATE ULTRA_WOPR_GUI)
  target_include_directories(wopr SYSTEM PRIVATE
    ${PROJECT_SOURCE_DIR}/third_party/imgui
    ${PROJECT_SOURCE_DIR}/third_party/implot
  )
  target_link_libraries(wopr PRIVATE ultra implot Threads::Threads)
else()
  target_link_libraries(wopr PRIVATE ultra Threads::Threads)
endif()

install(TARGETS wopr RUNTIME DESTINATION bin)
