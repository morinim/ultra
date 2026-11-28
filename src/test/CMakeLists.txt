# Creates the tests.

enable_testing()

file(GLOB TESTS_SRC CONFIGURE_DEPENDS *.cc)
foreach (test_src ${TESTS_SRC})
  get_filename_component(test ${test_src} NAME_WE)
  add_executable(${test} ${test_src})
  target_link_libraries(${test} PRIVATE ultra)

  if (NOT ${test} MATCHES "^speed_")
    add_test(NAME ${test} COMMAND ${test})
    list(APPEND registered_tests ${test})
  endif()
endforeach()

foreach (wopr_test wopr_monitor_data wopr_monitor wopr_results)
  string(REPLACE "wopr_" "wopr/" wopr_src "${wopr_test}.cc")
  add_executable(${wopr_test} ${wopr_src})
  target_link_libraries(${wopr_test} PRIVATE wopr_core)
  add_test(NAME ${wopr_test} COMMAND ${wopr_test})
  list(APPEND registered_tests ${wopr_test})
endforeach()

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

# Resources needed by tests
foreach (resource
         wopr/checksum_summary.py
         wopr/merge_summary.py
         test/wopr/check_monitor.js
         test/wopr/check_tooltips.js
         test/wopr/test_harness.js)
  get_filename_component(resource_name "${resource}" NAME)
  configure_file("${PROJECT_SOURCE_DIR}/${resource}"
                 "${CMAKE_CURRENT_BINARY_DIR}/${resource_name}"
                 COPYONLY)
endforeach()
