project(kate)

# Load the frameworks we need
find_package(KF5 CONFIG REQUIRED COMPONENTS DBusAddons)

add_subdirectory( app )
add_subdirectory( data )
