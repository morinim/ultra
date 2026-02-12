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

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      set_tests_properties(${test} PROPERTIES ENVIRONMENT
        "TSAN_OPTIONS=suppressions=${PROJECT_SOURCE_DIR}/utility/tsan.supp")
    endif()
  endif()
endforeach()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message("libc++ and TSAN, enabling ${PROJECT_SOURCE_DIR}/utility/tsan.supp")
endif()

# Resources needed by examples
file(COPY ${PROJECT_SOURCE_DIR}/wopr/merge_summary.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
