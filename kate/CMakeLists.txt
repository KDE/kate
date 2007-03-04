project(kate)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/interfaces)

add_subdirectory( interfaces )
add_subdirectory( app )
add_subdirectory( plugins )
add_subdirectory( data )
add_subdirectory( pics )

if( Q_WS_X11 )
   add_subdirectory(kicker)
endif(Q_WS_X11)

install( FILES kate_export.h DESTINATION ${INCLUDE_INSTALL_DIR} )
