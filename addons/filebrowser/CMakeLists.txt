find_package(KF5KIO ${KF5_DEP_VERSION} QUIET)
find_package(KF5Bookmarks ${KF5_DEP_VERSION} QUIET)
set_package_properties(KF5KIO PROPERTIES PURPOSE "Required to build the filebrowser addon")

if(NOT KF5KIO_FOUND)
  return()
endif()

kcoreaddons_add_plugin(katefilebrowserplugin INSTALL_NAMESPACE "ktexteditor")
target_compile_definitions(katefilebrowserplugin PRIVATE TRANSLATION_DOMAIN="katefilebrowserplugin")

target_link_libraries(
  katefilebrowserplugin 
  PRIVATE
    KF5::I18n
    KF5::TextEditor 
    KF5::KIOFileWidgets
    KF5::Bookmarks
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

