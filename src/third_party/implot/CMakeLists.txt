# Creates the "implot" library.

# --------- IMPLOT library ---------
set(IMPLOT_SRC implot.cpp implot_demo.cpp implot_items.cpp)

add_library(implot ${IMPLOT_SRC})

target_compile_options(implot PRIVATE ${NO_WARN_FLAGS})

target_include_directories(implot SYSTEM PRIVATE
  ${PROJECT_SOURCE_DIR}/third_party/imgui
  ${PROJECT_SOURCE_DIR}/third_party/implot
)

target_link_libraries(implot PUBLIC imgui)
