include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)

remove_definitions ( -DKDE_DEFAULT_DEBUG_AREA=13001 )
add_definitions ( -DKDE_DEFAULT_DEBUG_AREA=13040 )

remove_definitions(-DQT3_SUPPORT)

if(NOT WIN32)
   add_subdirectory( gdbplugin )   # reading /proc/...
endif(NOT WIN32)

add_subdirectory( textfilter )
add_subdirectory( konsole )

add_subdirectory( filebrowser )
add_subdirectory( mailfiles )

add_subdirectory( tabbarextension )
add_subdirectory( symbolviewer )
add_subdirectory( backtracebrowser )

add_subdirectory( filetemplates )

add_subdirectory( kate-ctags )
add_subdirectory( katebuild-plugin )
add_subdirectory( search )

add_subdirectory( kttsd )

add_subdirectory( openheader )
add_subdirectory( xmltools )
add_subdirectory( xmlcheck )

add_subdirectory( snippets )

add_subdirectory( tabify )

add_subdirectory( katesql )

add_subdirectory( filetree )

macro_optional_add_subdirectory(pate)

macro_optional_add_subdirectory(project)
