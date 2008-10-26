include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)

remove_definitions ( -DKDE_DEFAULT_DEBUG_AREA=13001 )
add_definitions ( -DKDE_DEFAULT_DEBUG_AREA=13040 )

if(NOT WIN32)
   add_subdirectory( konsole )
   add_subdirectory( textfilter )   # k3process
endif(NOT WIN32)

add_subdirectory( externaltools )
add_subdirectory( filebrowser )
add_subdirectory( findinfiles )
add_subdirectory( mailfiles )

add_subdirectory( tabbarextension )
add_subdirectory( symbolviewer )
add_subdirectory( backtracebrowser )

add_subdirectory( quickdocumentswitcher )

add_subdirectory( filetemplates )

add_subdirectory( kate-ctags )
add_subdirectory( katebuild-plugin )


#add_subdirectory( htmltools )
#add_subdirectory( kpybrowser )
add_subdirectory( openheader )
#add_subdirectory( xmltools )
#add_subdirectory( xmlcheck )
if(NOT WIN32)
    add_subdirectory( snippets )
endif(NOT WIN32)
MESSAGE( STATUS "remember to find maintainers for these plugins and port them to the kate 3.0 interfaces...")

# Example plugin, not meant to be built for production use.
#add_subdirectory( helloworld )
