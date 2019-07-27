find_package(KF5ItemViews QUIET)
set_package_properties(KF5ItemViews PROPERTIES PURPOSE "Required to build the search addon")

if(NOT KF5ItemViews_FOUND)
  return()
endif()

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
    KF5::ItemViews
    KF5::TextEditor
)

kcoreaddons_desktop_to_json(katesearchplugin katesearch.desktop)
install(TARGETS katesearchplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
