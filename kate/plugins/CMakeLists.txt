include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)

if(NOT WIN32)
   add_subdirectory( konsole )
endif(NOT WIN32)

add_subdirectory( externaltools )
add_subdirectory( filebrowser )
add_subdirectory( findinfiles )
add_subdirectory( mailfiles )

add_subdirectory( tabbarextension )
add_subdirectory( symbolviewer )

#add_subdirectory( htmltools )
#add_subdirectory( kpybrowser )
add_subdirectory( openheader )
add_subdirectory( textfilter )
#add_subdirectory( xmltools )
#add_subdirectory( xmlcheck )
#add_subdirectory( filetemplates )
#add_subdirectory( snippets )
MESSAGE( STATUS "remember to find maintainers for this plugins and port them to the kate 3.0 interfaces...")
