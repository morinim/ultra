# Creates the WOPR executable.

find_package(Threads REQUIRED)

file(GLOB WOPR_SRC CONFIGURE_DEPENDS "*.cc")
add_executable(wopr ${WOPR_SRC})

target_include_directories(wopr SYSTEM PRIVATE
  ${PROJECT_SOURCE_DIR}/third_party/imgui
  ${PROJECT_SOURCE_DIR}/third_party/implot
)

target_link_libraries(wopr PRIVATE ultra implot Threads::Threads)
