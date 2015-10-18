# detect additional frameworks
find_package(KF5 "${KF5_DEP_VERSION}" OPTIONAL_COMPONENTS Wallet Plasma Service ItemModels Notifications ThreadWeaver NewStuff IconThemes GuiAddons)

set_package_properties(KF5Wallet PROPERTIES PURPOSE "Required to build the katesql addon")
set_package_properties(KF5Plasma PROPERTIES PURPOSE "Required to build the sessionapplet addon")
set_package_properties(KF5Service PROPERTIES PURPOSE "Required to build the sessionapplet addon")
set_package_properties(KF5ItemModels PROPERTIES PURPOSE "Required to build the project, konsole addon")
set_package_properties(KF5Notifications PROPERTIES PURPOSE "Required to build the katebuild, close-except-like addons")
set_package_properties(KF5ThreadWeaver PROPERTIES PURPOSE "Required to build the project addon")
set_package_properties(KF5NewStuff PROPERTIES PURPOSE "Required to build the snippets addon")

# document switcher
ecm_optional_add_subdirectory (filetree)

# search in open documents and files
ecm_optional_add_subdirectory (search)

# ALT+Tab like tab switcher
ecm_optional_add_subdirectory (tabswitcher)

# ctags
ecm_optional_add_subdirectory (kate-ctags)

# backtrace
ecm_optional_add_subdirectory (backtracebrowser)

# file browser, atm guarded until we have a check for KIOWidgets
if (NOT WIN32)
    ecm_optional_add_subdirectory (filebrowser)
endif()

# xml completion
ecm_optional_add_subdirectory (xmltools)

# open header matching to current file
ecm_optional_add_subdirectory (openheader)

# debugger plugin, needs windows love, guarded until ported to win32
if (NOT WIN32)
    ecm_optional_add_subdirectory (gdbplugin)
endif ()

# list symbols and functions in a file
ecm_optional_add_subdirectory (symbolviewer)

# replicode integration
ecm_optional_add_subdirectory (replicode)

# pipe text through some external command
ecm_optional_add_subdirectory (textfilter)

# Rust complection plugin
ecm_optional_add_subdirectory (rustcompletion)

# D completion plugin
ecm_optional_add_subdirectory (lumen)

# build plugin
ecm_optional_add_subdirectory (katebuild-plugin)

if(KF5Notifications_FOUND)
    # close document except this one (or similar)
    ecm_optional_add_subdirectory (close-except-like)
endif()

if(KF5Wallet_FOUND)
    # kate sql
    ecm_optional_add_subdirectory (katesql)
endif()

if(KF5NewStuff_FOUND)
    # snippets
    ecm_optional_add_subdirectory (snippets)
endif()

if(KF5ItemModels_FOUND)
    # terminal tool view
    ecm_optional_add_subdirectory (konsole)
endif()

if(KF5ItemModels_FOUND AND KF5ThreadWeaver_FOUND)
    # small & smart project manager
    ecm_optional_add_subdirectory (project)
endif()

if (KF5Plasma_FOUND AND KF5Service_FOUND)
    ecm_optional_add_subdirectory (sessionapplet)
endif()
