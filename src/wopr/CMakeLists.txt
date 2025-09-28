# Creates the WOPR executable.

find_package(Threads)

include_directories(SYSTEM ${ULTRA_SOURCE_DIR}/third_party/imgui)
include_directories(SYSTEM ${ULTRA_SOURCE_DIR}/third_party/implot)

file(GLOB WOPR_SRC CONFIGURE_DEPENDS "*.cc")

add_executable(wopr ${WOPR_SRC})
target_link_libraries(wopr ultra imgui implot ${CMAKE_THREAD_LIBS_INIT})
