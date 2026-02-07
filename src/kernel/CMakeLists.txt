# Creates the "ultra" library.

get_filename_component(ULTRA_REPO_ROOT
  "${PROJECT_SOURCE_DIR}/.."
  ABSOLUTE
)

file(GLOB_RECURSE KERNEL_SRC CONFIGURE_DEPENDS "*.cc")
file(GLOB_RECURSE UTILITY_SRC CONFIGURE_DEPENDS "../utility/*.cc")

set(FRAMEWORK_SRC ${KERNEL_SRC} ${UTILITY_SRC})

add_library(ultra ${FRAMEWORK_SRC})

target_include_directories(ultra PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(ultra SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/third_party)

target_link_libraries(ultra PUBLIC tinyxml2)

# Required by libstdc++ on MinGW for C++23 <print>.
if (MINGW)
  target_link_libraries(ultra PRIVATE stdc++exp)
endif()

add_custom_command(TARGET ultra POST_BUILD
  COMMAND ${ULTRA_REPO_ROOT}/tools/single_include.py
          --src-include-dir ./
          --src-include kernel/ultra.h
          --dst-include ${CMAKE_CURRENT_BINARY_DIR}/auto_ultra.h
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  VERBATIM
)

# Define GNU standard installation directories.
include(GNUInstallDirs)

# Installation rules for the project.
install(TARGETS ultra)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/auto_ultra.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        RENAME ultra.h)
