project(kate)

# Load the frameworks we need
find_package(KF5 CONFIG REQUIRED COMPONENTS DBusAddons)

add_definitions ( -DKDE_DEFAULT_DEBUG_AREA=13001 )

add_subdirectory( app )
add_subdirectory( data )
#add_subdirectory( filetree )