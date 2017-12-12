add_definitions(-DTRANSLATION_DOMAIN=\"katesearch\")

set(katesearchplugin_PART_SRCS
    plugin_search.cpp
    search_open_files.cpp
    SearchDiskFiles.cpp
    FolderFilesList.cpp
    replace_matches.cpp
    htmldelegate.cpp
)

ki18n_wrap_ui (katesearchplugin_PART_SRCS search.ui results.ui)

# resource for ui file and stuff
qt5_add_resources(katesearchplugin_PART_SRCS plugin.qrc)

add_library(katesearchplugin MODULE ${katesearchplugin_PART_SRCS})
kcoreaddons_desktop_to_json (katesearchplugin katesearch.desktop)
target_link_libraries(katesearchplugin
    KF5::TextEditor
    KF5::Parts KF5::I18n KF5::IconThemes
    KF5::ItemViews)

install(TARGETS katesearchplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
