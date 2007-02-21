include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)

if(NOT WIN32)
   add_subdirectory( konsole )
endif(NOT WIN32)

add_subdirectory( externaltools )
add_subdirectory( filebrowser )
add_subdirectory( findinfiles )
add_subdirectory( mailfiles )


