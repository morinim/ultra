# Creates the "implot" library.

# --------- IMPLOT library ---------
set(IMPLOT_SRC implot.cpp implot_demo.cpp implot_items.cpp)

add_compile_options(${NO_WARN_FLAGS})

add_library(implot ${IMPLOT_SRC})

include_directories(SYSTEM ${ULTRA_SOURCE_DIR}/third_party/imgui)
include_directories(SYSTEM ${ULTRA_SOURCE_DIR}/third_party/implot)
