# Creates the tests.

find_package(Threads)

enable_testing()

file(GLOB TESTS_SRC CONFIGURE_DEPENDS *.cc)
foreach (test_src ${TESTS_SRC})
  get_filename_component(test ${test_src} NAME_WE)
  add_executable(${test} ${test_src})
  target_link_libraries(${test} ultra ${CMAKE_THREAD_LIBS_INIT})

  if (NOT ${test} MATCHES "^speed_")
    add_test(NAME ${test} COMMAND ${test})
  endif()
endforeach()
