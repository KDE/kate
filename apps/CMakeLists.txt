# kateprivate shared library used by Kate & KWrite (and addons)
add_subdirectory(lib)

add_definitions(-DKDE_INSTALL_FULL_LOCALEDIR=\"${KDE_INSTALL_FULL_LOCALEDIR}\")

# the Kate application
ecm_optional_add_subdirectory(kate)

# the KWrite application
ecm_optional_add_subdirectory(kwrite)
