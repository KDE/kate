find_package(Qt5Concurrent ${QT_MIN_VERSION} QUIET)
find_package(KF5ItemViews ${KF5_DEP_VERSION} QUIET)
set_package_properties(KF5ItemViews PROPERTIES PURPOSE "Required to build the search addon")

if(NOT KF5ItemViews_FOUND OR NOT Qt5Concurrent_FOUND)
  return()
endif()

kate_add_plugin(katesearchplugin)
target_compile_definitions(katesearchplugin PRIVATE TRANSLATION_DOMAIN="katesearch")

target_link_libraries(
  katesearchplugin
  PRIVATE
    Qt5::Concurrent
    KF5::I18n
    KF5::ItemViews
    KF5::TextEditor
)

ki18n_wrap_ui(katesearchplugin search.ui results.ui MatchExportDialog.ui)

target_sources(
  katesearchplugin
  PRIVATE
    FolderFilesList.cpp
    KateSearchCommand.cpp
    MatchExportDialog.cpp
    MatchModel.cpp
    MatchProxyModel.cpp
    SearchDiskFiles.cpp
    htmldelegate.cpp
    plugin.qrc
    plugin_search.cpp
    search_open_files.cpp
    Results.cpp
)

