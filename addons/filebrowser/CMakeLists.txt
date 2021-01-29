find_package(KF5KIO ${KF5_DEP_VERSION} QUIET)
set_package_properties(KF5KIO PROPERTIES PURPOSE "Required to build the filebrowser addon")

if(NOT KF5KIO_FOUND)
  return()
endif()

add_library(katefilebrowserplugin MODULE "")
target_compile_definitions(katefilebrowserplugin PRIVATE TRANSLATION_DOMAIN="katefilebrowserplugin")

target_link_libraries(
  katefilebrowserplugin 
  PRIVATE
    KF5::TextEditor 
    KF5::KIOFileWidgets
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

kcoreaddons_desktop_to_json(katefilebrowserplugin katefilebrowserplugin.desktop)
install(TARGETS katefilebrowserplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/ktexteditor)
