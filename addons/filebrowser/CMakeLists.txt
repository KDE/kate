find_package(KF${KF_MAJOR_VERSION}KIO ${KF5_DEP_VERSION} QUIET)
find_package(KF${KF_MAJOR_VERSION}Bookmarks ${KF5_DEP_VERSION} QUIET)
set_package_properties(KF${KF_MAJOR_VERSION}KIO PROPERTIES PURPOSE "Required to build the filebrowser addon")

if(NOT KF5KIO_FOUND)
  return()
endif()

kate_add_plugin(katefilebrowserplugin)
target_compile_definitions(katefilebrowserplugin PRIVATE TRANSLATION_DOMAIN="katefilebrowserplugin")

target_link_libraries(
  katefilebrowserplugin 
  PRIVATE
    kateprivate
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::TextEditor 
    KF${KF_MAJOR_VERSION}::KIOFileWidgets
    KF${KF_MAJOR_VERSION}::Bookmarks
)

target_sources(
  katefilebrowserplugin
  PRIVATE
    katefilebrowserplugin.cpp
    katefilebrowserconfig.cpp
    katefilebrowser.cpp
    katebookmarkhandler.cpp
    katefilebrowseropenwithmenu.cpp
)

