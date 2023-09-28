find_package(Qt6Concurrent ${QT_MIN_VERSION} QUIET)

if(NOT Qt6Concurrent_FOUND)
  return()
endif()

kate_add_plugin(katesearchplugin)
target_compile_definitions(katesearchplugin PRIVATE TRANSLATION_DOMAIN="katesearch")

target_link_libraries(
  katesearchplugin
  PRIVATE
    Qt::Concurrent
    KF6::I18n
    KF6::TextEditor
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
