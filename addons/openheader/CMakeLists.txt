project (kateopenheader)
find_package(KF5 REQUIRED COMPONENTS TextEditor
    KIO
    Parts
    XmlGui )

########### next target ###############
set(kateopenheaderplugin_PART_SRCS plugin_kateopenheader.cpp )

add_library(kateopenheaderplugin MODULE ${kateopenheaderplugin_PART_SRCS})

target_link_libraries(kateopenheaderplugin
  KF5::TextEditor
  KF5::KIOFileWidgets
  KF5::I18n
  KF5::Parts)

install(TARGETS kateopenheaderplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )

########### install files ###############
kcoreaddons_desktop_to_json (kateopenheaderplugin kateopenheaderplugin.desktop)
install( FILES kateopenheaderplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install(TARGETS kateopenheaderplugin  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )

install( FILES ui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/kateopenheaderplugin )
