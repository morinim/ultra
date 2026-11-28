# Creates the examples.

file(GLOB EXAMPLES_SRC CONFIGURE_DEPENDS *.cc 8queens/*.cc knapsack/*.cc nonogram/*.cc pathfinding/*.cc petalrose/*.cc polyomino/*.cc sonar/*.cc symbolic_regression/*.cc whichlang/*.cc)
foreach (example_src ${EXAMPLES_SRC})
  # Gets the filename (`example`) without directory / longest extension
  # (`NAME_WE`) from the full filename (${example_src})
  get_filename_component(example ${example_src} NAME_WE)

  add_executable(${example} ${example_src})
  target_link_libraries(${example} PRIVATE ultra)
endforeach()

# Resources needed by examples
foreach (resource sonar.csv petalrose.csv)
  configure_file("${PROJECT_SOURCE_DIR}/../data/${resource}"
                 "${CMAKE_CURRENT_BINARY_DIR}/${resource}"
                 COPYONLY)
endforeach()
