project(kate)

add_definitions ( -DKDE_DEFAULT_DEBUG_AREA=13001 )

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/interfaces)
macro_optional_find_package(Plasma)
macro_bool_to_01(PLASMA_FOUND HAVE_PLASMA)

add_subdirectory( interfaces/kate )
add_subdirectory( app )
add_subdirectory( plugins )
add_subdirectory( data )
add_subdirectory( pics )
 
macro_log_feature(PLASMA_FOUND "Plasma" "Plasma is the KDE4 desktop" "http://plasma.kde.org" FALSE "" "Provides a Kate Plasmoid")
if( PLASMA_FOUND )
   add_subdirectory(plasma)
endif(PLASMA_FOUND)

install( FILES kate_export.h DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel)
