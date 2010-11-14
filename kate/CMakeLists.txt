project(kate)

add_definitions ( -DKDE_DEFAULT_DEBUG_AREA=13001 )

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/interfaces)

add_subdirectory( interfaces/kate )
add_subdirectory( app )
add_subdirectory( plugins )
add_subdirectory( data )
add_subdirectory( pics )
 
add_subdirectory(plasma)

install( FILES kate_export.h DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel)
