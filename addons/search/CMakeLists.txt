find_package(KF5ItemViews ${KF5_DEP_VERSION} QUIET)
set_package_properties(KF5ItemViews PROPERTIES PURPOSE "Required to build the search addon")

if(NOT KF5ItemViews_FOUND)
  return()
endif()

add_library(katesearchplugin MODULE "")
target_compile_definitions(katesearchplugin PRIVATE TRANSLATION_DOMAIN="katesearch")

target_link_libraries(
  katesearchplugin 
  PRIVATE
    KF5::ItemViews
    KF5::TextEditor
)

ki18n_wrap_ui(UI_SOURCES search.ui results.ui)
target_sources(katesearchplugin PRIVATE ${UI_SOURCES})

target_sources(
  katesearchplugin
  PRIVATE
    FolderFilesList.cpp
    KateSearchCommand.cpp
    MatchModel.cpp
    SearchDiskFiles.cpp
    htmldelegate.cpp
    plugin.qrc
    plugin_search.cpp
    search_open_files.cpp
)

kcoreaddons_desktop_to_json(katesearchplugin katesearch.desktop)
install(TARGETS katesearchplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/ktexteditor)
