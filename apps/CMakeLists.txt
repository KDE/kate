# kateprivate shared library used by Kate & KWrite (and addons)
add_subdirectory(lib)

# the Kate application
ecm_optional_add_subdirectory(kate)

# the KWrite application
ecm_optional_add_subdirectory(kwrite)
