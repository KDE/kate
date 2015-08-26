#
# The Kate Application
#
project(kate)

# Load the frameworks we need
find_package(KF5 REQUIRED COMPONENTS DBusAddons GuiAddons IconThemes)

# .desktop files and co
add_subdirectory (data)

# icons
add_subdirectory (icons)

# the app itself
add_subdirectory (src)

# automatic unit tests
ecm_optional_add_subdirectory (autotests)
