# Creates the "echarts_bundle" library embedding Apache ECharts.

set(ECHARTS_JS "${CMAKE_CURRENT_SOURCE_DIR}/echarts.min.js")
set(ECHARTS_CC "${CMAKE_CURRENT_BINARY_DIR}/echarts_bundle.cc")

add_custom_command(
  OUTPUT "${ECHARTS_CC}"
  COMMAND ${CMAKE_COMMAND}
    -DINPUT=${ECHARTS_JS}
    -DOUTPUT=${ECHARTS_CC}
    -P "${CMAKE_CURRENT_SOURCE_DIR}/generate_bundle.cmake"
  DEPENDS "${ECHARTS_JS}" "${CMAKE_CURRENT_SOURCE_DIR}/generate_bundle.cmake"
  COMMENT "Generating embedded Apache ECharts bundle"
  VERBATIM
)

add_library(echarts_bundle STATIC "${ECHARTS_CC}")
target_include_directories(echarts_bundle PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(echarts_bundle PRIVATE ${NO_WARN_FLAGS})
