# Creates the WOPR executable.

include_directories(SYSTEM ${ULTRA_SOURCE_DIR}/third_party/imgui)

find_package(Threads)

file(GLOB WOPR_SRC CONFIGURE_DEPENDS "*.cc")

add_executable(wopr ${WOPR_SRC})
target_link_libraries(wopr ultra imgui)
