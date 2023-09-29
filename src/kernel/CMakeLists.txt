# Creates the "ultra" library.

file(GLOB_RECURSE KERNEL_SRC CONFIGURE_DEPENDS "*.cc")
file(GLOB_RECURSE UTILITY_SRC CONFIGURE_DEPENDS "../utility/*.cc")

set(FRAMEWORK_SRC ${KERNEL_SRC} ${UTILITY_SRC})

add_library(ultra ${FRAMEWORK_SRC})

#target_link_libraries(ultra tinyxml2)

#add_custom_command(TARGET ultra POST_BUILD
#                   COMMAND ../tools/single_include.py --src-include-dir ./ --src-include kernel/ultra.h --dst-include ${CMAKE_CURRENT_BINARY_DIR}/auto_ultra.h
#				   WORKING_DIRECTORY ${ULTRA_SOURCE_DIR}
#				   VERBATIM)

#include(GNUInstallDirs)
#install(TARGETS ultra)
#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/auto_ultra.h
#        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#		RENAME ultra.h)
