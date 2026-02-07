# Creates the "tinyxml2" library.

set(TINYXML2_SRC tinyxml2.cpp)

add_library(tinyxml2 ${TINYXML2_SRC})

target_compile_options(tinyxml2 PRIVATE ${NO_WARN_FLAGS})


