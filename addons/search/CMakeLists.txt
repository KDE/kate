find_package(Qt${QT_MAJOR_VERSION}Concurrent ${QT_MIN_VERSION} QUIET)
find_package(KF${KF_MAJOR_VERSION}ItemViews ${KF5_DEP_VERSION} QUIET)
set_package_properties(KF${KF_MAJOR_VERSION}ItemViews PROPERTIES PURPOSE "Required to build the search addon")

if(NOT KF${KF_MAJOR_VERSION}ItemViews_FOUND OR NOT Qt${QT_MAJOR_VERSION}Concurrent_FOUND)
  return()
endif()

kate_add_plugin(katesearchplugin)
target_compile_definitions(katesearchplugin PRIVATE TRANSLATION_DOMAIN="katesearch")

target_link_libraries(
  katesearchplugin
  PRIVATE
    Qt::Concurrent
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::ItemViews
    KF${KF_MAJOR_VERSION}::TextEditor
    kateprivate
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
    SearchResultsDelegate.cpp
    plugin.qrc
    SearchPlugin.cpp
    SearchOpenFiles.cpp
    Results.cpp
    ResultsTreeView.cpp
)

if (BUILD_PCH)
    target_precompile_headers(katesearchplugin REUSE_FROM katepch)
endif()
