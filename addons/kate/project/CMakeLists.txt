project(kateprojectplugin)

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

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

target_link_libraries(kateprojectplugin kateinterfaces ktexteditor
    KF5::Parts
    KF5::KDE4Support
    KF5::NewStuff Qt5::Script KF5::SonnetCore KF5::Archive KF5::KCMUtils KF5::PrintUtils KF5::ItemViews KF5::ItemModels)

########### install files ###############
install(TARGETS kateprojectplugin DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/project )

kservice_desktop_to_json (kateprojectplugin.desktop)
install( FILES kateprojectplugin.desktop DESTINATION  ${SERVICES_INSTALL_DIR} )
