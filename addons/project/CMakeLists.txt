project(kateprojectplugin)

# Load the frameworks we need
find_package(KF5 REQUIRED COMPONENTS ItemModels)

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_definitions(-DTRANSLATION_DOMAIN=\"kateproject\")
set(kateprojectplugin_PART_SRCS
  kateprojectplugin.cpp
  kateprojectpluginview.cpp
  kateproject.cpp
  kateprojectworker.cpp
  kateprojectitem.cpp
  kateprojectview.cpp
  kateprojectviewtree.cpp
  kateprojecttreeviewcontextmenu.cpp
  kateprojectinfoview.cpp
  kateprojectcompletion.cpp
  kateprojectindex.cpp
  kateprojectinfoviewindex.cpp
  kateprojectinfoviewterminal.cpp
  kateprojectinfoviewcodeanalysis.cpp
  kateprojectinfoviewnotes.cpp
)

add_library (kateprojectplugin MODULE ${kateprojectplugin_PART_SRCS})

target_link_libraries(kateprojectplugin
    KF5::TextEditor
    KF5::Parts KF5::I18n
    Qt5::Script KF5::ItemViews KF5::ItemModels KF5::IconThemes)

########### install files ###############
install(TARGETS kateprojectplugin DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES ui.rc DESTINATION  ${KXMLGUI_INSTALL_DIR}/kateproject )
install( FILES kateproject.example DESTINATION  ${DATA_INSTALL_DIR}/kateproject )

kservice_desktop_to_json (kateprojectplugin kateprojectplugin.desktop)
install( FILES kateprojectplugin.desktop DESTINATION  ${SERVICES_INSTALL_DIR} )
