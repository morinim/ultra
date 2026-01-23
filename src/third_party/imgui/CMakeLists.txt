# Creates the "imgui" library.

# --------- IMGUI library ---------
set(IMGUI_SRC imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp backends/imgui_impl_sdl3.cpp backends/imgui_impl_sdlrenderer3.cpp)

add_compile_options(${NO_WARN_FLAGS})

add_library(imgui ${IMGUI_SRC})

include_directories(SYSTEM ${ULTRA_SOURCE_DIR}/third_party/imgui)

# --------- SDL3 library ---------
# Look for a SDL3 package.
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

# Link to the actual SDL3 library. SDL3::SDL3 is the shared SDL library,
# SDL3::SDL3-static is the static SDL library.
target_link_libraries(imgui PRIVATE SDL3::SDL3)
