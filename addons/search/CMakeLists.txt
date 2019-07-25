set(katesearchplugin_PART_SRCS
  plugin_search.cpp
  search_open_files.cpp
  SearchDiskFiles.cpp
  FolderFilesList.cpp
  replace_matches.cpp
  htmldelegate.cpp
  plugin.qrc
)

ki18n_wrap_ui(katesearchplugin_PART_SRCS search.ui results.ui)

add_library(katesearchplugin MODULE ${katesearchplugin_PART_SRCS})
target_compile_definitions(katesearchplugin PRIVATE TRANSLATION_DOMAIN="katesearch")

target_link_libraries(
  katesearchplugin 
  PRIVATE
    KF5::TextEditor
    KF5::Parts KF5::I18n KF5::IconThemes
    KF5::ItemViews
)

kcoreaddons_desktop_to_json(katesearchplugin katesearch.desktop)
install(TARGETS katesearchplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
