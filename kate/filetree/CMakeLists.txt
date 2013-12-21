
project(katefiletreeplugin)

set(katefiletreeplugin_PART_SRCS katefiletreeplugin.cpp katefiletree.cpp katefiletreemodel.cpp katefiletreeproxymodel.cpp katefiletreeconfigpage.cpp katefiletreepluginsettings.cpp )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_library (katefiletreeplugin MODULE  ${katefiletreeplugin_PART_SRCS})

target_link_libraries(katefiletreeplugin kateinterfaces ktexteditor
  KF5::I18n
  KF5::GuiAddons
  KF5::IconThemes
)

install(TARGETS katefiletreeplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )

kservice_desktop_to_json (katefiletreeplugin.desktop)
install( FILES katefiletreeplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/filetree )