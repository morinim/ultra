# Creates the tests.

find_package(Threads REQUIRED)

enable_testing()

file(GLOB TESTS_SRC CONFIGURE_DEPENDS *.cc)
foreach (test_src ${TESTS_SRC})
  get_filename_component(test ${test_src} NAME_WE)
  add_executable(${test} ${test_src})
  target_link_libraries(${test} PRIVATE ultra Threads::Threads)

  if (NOT ${test} MATCHES "^speed_")
    add_test(NAME ${test} COMMAND ${test})
    list(APPEND registered_tests ${test})
  endif()
endforeach()

add_executable(wopr_monitor_data
  wopr/monitor_data.cc
)
target_link_libraries(wopr_monitor_data PRIVATE
  ultra Threads::Threads
)
add_test(NAME wopr_monitor_data COMMAND wopr_monitor_data)
list(APPEND registered_tests wopr_monitor_data)

if (TARGET implot)
  add_executable(wopr_monitor
    wopr/monitor.cc
    ../wopr/dashboard.cc
    ../wopr/imgui_app.cc
    ../wopr/monitor_data.cc
  )
  target_compile_definitions(wopr_monitor PRIVATE ULTRA_WOPR_GUI)
  target_include_directories(wopr_monitor SYSTEM PRIVATE
    ${PROJECT_SOURCE_DIR}/third_party/imgui
  )
  target_link_libraries(wopr_monitor PRIVATE
    ultra implot SDL3::SDL3 Threads::Threads
  )
  add_test(NAME wopr_monitor COMMAND wopr_monitor)
  list(APPEND registered_tests wopr_monitor)

  add_executable(wopr_results
    wopr/results.cc
    ../wopr/dashboard.cc
    ../wopr/imgui_app.cc
  )
  target_compile_definitions(wopr_results PRIVATE ULTRA_WOPR_GUI)
  target_include_directories(wopr_results SYSTEM PRIVATE
    ${PROJECT_SOURCE_DIR}/third_party/imgui
  )
  target_link_libraries(wopr_results PRIVATE
    ultra implot SDL3::SDL3 Threads::Threads
  )
  add_test(NAME wopr_results COMMAND wopr_results)
  list(APPEND registered_tests wopr_results)
endif()

get_directory_property(compile_options COMPILE_OPTIONS)
string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type_upper)
string(JOIN " " all_build_flags
  "${CMAKE_CXX_FLAGS}"
  "${CMAKE_CXX_FLAGS_${build_type_upper}}"
  "${CMAKE_EXE_LINKER_FLAGS}"
  "${OTHER_FLAGS}"
  "${compile_options}"
  "$ENV{SANITISER_FLAGS}"
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND all_build_flags MATCHES "-fsanitize=thread")
  set_tests_properties(${registered_tests} PROPERTIES ENVIRONMENT
    "TSAN_OPTIONS=suppressions=${PROJECT_SOURCE_DIR}/utility/tsan.supp")
  message(STATUS "libc++ and TSAN, enabling ${PROJECT_SOURCE_DIR}/utility/tsan.supp")
endif()

# Resources needed by examples
file(COPY ${PROJECT_SOURCE_DIR}/wopr/checksum_summary.py ${PROJECT_SOURCE_DIR}/wopr/merge_summary.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
