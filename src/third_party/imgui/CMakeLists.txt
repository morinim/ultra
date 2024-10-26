# Creates the "imgui" library.

# --------- IMGUI library ---------
set(IMGUI_SRC imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp backends/imgui_impl_sdl2.cpp backends/imgui_impl_sdlrenderer2.cpp)

add_compile_options(${NO_WARN_FLAGS})

add_library(imgui ${IMGUI_SRC})

include_directories(SYSTEM ${ULTRA_SOURCE_DIR}/third_party/imgui)

# --------- SDL2 library ---------
# Look for a SDL2 package, for the SDL2 component and fail if none can be found.
find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)

# Look for a SDL2 package, for the SDL2maincomponent and DO NOT fail when SDL2main is not available.
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)

# SDL2::SDL2main may or may not be available (it's required by Windows GUI applications).
if(TARGET SDL2::SDL2main)
    # It has an implicit dependency on SDL2 functions, so it MUST be added before SDL2::SDL2 (or SDL2::SDL2-static).
    target_link_libraries(imgui PRIVATE SDL2::SDL2main)
endif()

# Link to the actual SDL2 library. SDL2::SDL2 is the shared SDL library, SDL2::SDL2-static is the static SDL library.
target_link_libraries(imgui PRIVATE SDL2::SDL2)
