project(kate)

# Load the frameworks we need
find_package(KF5 CONFIG REQUIRED COMPONENTS DBusAddons)

add_definitions ( -DKDE_DEFAULT_DEBUG_AREA=13001 )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/interfaces)

add_subdirectory( interfaces/kate )
add_subdirectory( app )
add_subdirectory( filetree )
add_subdirectory( data )
