set(katesearchplugin_PART_SRCS
    plugin_search.cpp
    search_open_files.cpp
    SearchDiskFiles.cpp
    FolderFilesList.cpp
    replace_matches.cpp
    htmldelegate.cpp
)

ki18n_wrap_ui (katesearchplugin_PART_SRCS search.ui results.ui)

add_library(katesearchplugin MODULE ${katesearchplugin_PART_SRCS})

target_link_libraries(katesearchplugin
    KF5::TextEditor
    KF5::Parts KF5::I18n KF5::IconThemes
    Qt5::Script KF5::ItemViews)

########### install files ###############
install(TARGETS katesearchplugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/katesearch)


kservice_desktop_to_json (katesearchplugin katesearch.desktop)
install(FILES katesearch.desktop DESTINATION ${SERVICES_INSTALL_DIR})
