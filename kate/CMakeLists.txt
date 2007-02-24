project(kate)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/interfaces)

add_subdirectory( interfaces )
add_subdirectory( app )
add_subdirectory( plugins )
add_subdirectory( data )
add_subdirectory( pics )

install( FILES kdebase_export.h DESTINATION ${INCLUDE_INSTALL_DIR} )
