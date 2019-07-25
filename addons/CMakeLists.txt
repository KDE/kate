find_package(
  KF5 ${KF5_DEP_VERSION} 
  OPTIONAL_COMPONENTS 
    GuiAddons
    IconThemes 
    ItemModels 
    NewStuff 
    Plasma
    Service 
    Wallet 
    ThreadWeaver
)

set_package_properties(KF5Wallet PROPERTIES PURPOSE "Required to build the katesql addon")
set_package_properties(KF5Plasma PROPERTIES PURPOSE "Required to build the sessionapplet addon")
set_package_properties(KF5Service PROPERTIES PURPOSE "Required to build the sessionapplet addon")
set_package_properties(KF5ItemModels PROPERTIES PURPOSE "Required to build the project, konsole addon")
set_package_properties(KF5ThreadWeaver PROPERTIES PURPOSE "Required to build the project addon")
set_package_properties(KF5NewStuff PROPERTIES PURPOSE "Required to build the snippets and project addons")

ecm_optional_add_subdirectory(filetree) # document switcher
ecm_optional_add_subdirectory(search) # Search in open documents and files.
ecm_optional_add_subdirectory(tabswitcher) # ALT+Tab like tab switcher
ecm_optional_add_subdirectory(kate-ctags) # ctags
ecm_optional_add_subdirectory(backtracebrowser) # backtrace
ecm_optional_add_subdirectory(filebrowser) # file browser
ecm_optional_add_subdirectory(xmltools) # XML completion
ecm_optional_add_subdirectory(xmlcheck) # XML Validation plugin
ecm_optional_add_subdirectory(openheader) # Open header matching to current file.
ecm_optional_add_subdirectory(symbolviewer) # List symbols and functions in a file.
ecm_optional_add_subdirectory(replicode) # replicode integration
ecm_optional_add_subdirectory(textfilter) # Pipe text through some external command.
ecm_optional_add_subdirectory(rustcompletion) # Rust complection plugin
ecm_optional_add_subdirectory(lumen) # D completion plugin
ecm_optional_add_subdirectory(lspclient) # Language Server Protocol (LSP) client plugin.
ecm_optional_add_subdirectory(katebuild-plugin) # build plugin
ecm_optional_add_subdirectory(close-except-like) # Close document except this one (or similar).
ecm_optional_add_subdirectory (preview) # Live preview of sources in target format.

if(NOT WIN32)
  ecm_optional_add_subdirectory(gdbplugin) # Debugger plugin. Hasn't been ported to Windows yet.
endif()

if(KF5Wallet_FOUND)  
  ecm_optional_add_subdirectory(katesql) # kate sql
endif()

if(KF5NewStuff_FOUND)
  ecm_optional_add_subdirectory (snippets) # snippets
endif()

if(KF5Service_FOUND AND NOT WIN32)
  ecm_optional_add_subdirectory (konsole) # terminal tool view
endif()

if(KF5ItemModels_FOUND AND KF5ThreadWeaver_FOUND AND KF5NewStuff_FOUND)    
  ecm_optional_add_subdirectory (project) # Small & smart project manager.
endif()

if(KF5Plasma_FOUND AND KF5Service_FOUND)
  ecm_optional_add_subdirectory (sessionapplet)
endif()
