# detect aditional frameworks
find_package(KF5 OPTIONAL_COMPONENTS Wallet Plasma Service ItemModels Notifications)

set_package_properties(KF5Wallet PROPERTIES PURPOSE "Required to build the katesql addon")
set_package_properties(KF5Plasma PROPERTIES PURPOSE "Required to build the sessionapplet addon")
set_package_properties(KF5Service PROPERTIES PURPOSE "Required to build the sessionapplet addon")
set_package_properties(KF5ItemModels PROPERTIES PURPOSE "Required to build the project, konsole addon")
set_package_properties(KF5Notifications PROPERTIES PURPOSE "Required to build the katebuild, close-except-like  addon")

if (NOT(KF5Plasma_FOUND AND KF5Service_FOUND))
    set(BUILD_sessionapplet OFF)
endif()

if(NOT(KF5ItemModels_FOUND))
    set(BUILD_project OFF)
    set(BUILD_konsole OFF)
endif()

if(NOT(KF5Wallet_FOUND))
    set(BUILD_katesql OFF)
endif()

if(NOT(KF5Notifications_FOUND))
    set(BUILD_katebuild-plugin OFF)
    set(BUILD_close-except-like OFF)
endif()

# document switcher
ecm_optional_add_subdirectory (filetree)

# search in open documents and files
ecm_optional_add_subdirectory (search)

# small & smart project manager
ecm_optional_add_subdirectory (project)

# terminal tool view
ecm_optional_add_subdirectory (konsole)

# ALT+Tab like tab switcher
ecm_optional_add_subdirectory (tabswitcher)

# snippets
ecm_optional_add_subdirectory (snippets)

# ctags
ecm_optional_add_subdirectory (kate-ctags)

# backtrace
ecm_optional_add_subdirectory (backtracebrowser)

# kate sql
ecm_optional_add_subdirectory (katesql)

# file browser
ecm_optional_add_subdirectory (filebrowser)

# xml completion
ecm_optional_add_subdirectory (xmltools)

# open header matching to current file
ecm_optional_add_subdirectory (openheader)

# close document except this one (or similar)
ecm_optional_add_subdirectory (close-except-like)

# debugger plugin, needs windows love
ecm_optional_add_subdirectory (gdbplugin)

# build plugin
ecm_optional_add_subdirectory (katebuild-plugin)

#add_subdirectory( textfilter )

#add_subdirectory( mailfiles )

#add_subdirectory( symbolviewer )

#add_subdirectory( filetemplates )

#add_subdirectory( kttsd )

#add_subdirectory( xmlcheck )

ecm_optional_add_subdirectory(sessionapplet)

#macro_optional_add_subdirectory(pate)
