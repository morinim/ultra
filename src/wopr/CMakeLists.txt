# Creates the WOPR executable and static library target.

set(WOPR_LIB_SRC
  command_line.cc
  html_monitor.cc
  html_report.cc
  html_shell.cc
  monitor.cc
  monitor_data.cc
  results.cc
  run.cc
)

add_library(wopr_core STATIC ${WOPR_LIB_SRC})
target_link_libraries(wopr_core PUBLIC ultra PRIVATE echarts_bundle)

add_executable(wopr wopr.cc)
target_link_libraries(wopr PRIVATE wopr_core)

install(TARGETS wopr RUNTIME DESTINATION bin)
