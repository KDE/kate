# Kate interfaces used for the plugins
include_directories (${CMAKE_SOURCE_DIR}/kate/interfaces)

# default debug area
add_definitions ( -DKDE_DEFAULT_DEBUG_AREA=13040 )

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

add_subdirectory( close-except-like )

add_subdirectory( replicode )

macro_optional_add_subdirectory(pate)

macro_optional_add_subdirectory(project)
