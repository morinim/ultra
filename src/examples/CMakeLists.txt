# Creates the examples.

find_package(Threads)

file(GLOB EXAMPLES_SRC CONFIGURE_DEPENDS *.cc nonogram/*.cc pathfinding/*.cc polyomino/*.cc sonar/*.cc symbolic_regression/*.cc)
foreach (example_src ${EXAMPLES_SRC})
  # Gets the filename (`example`) without directory / longest extension
  # (`NAME_WE`) from the full filename (${example_src})
  get_filename_component(example ${example_src} NAME_WE)

  add_executable(${example} ${example_src})
  target_link_libraries(${example} ultra ${CMAKE_THREAD_LIBS_INIT})
endforeach()

# Resources needed by examples
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sonar/sonar.csv
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
