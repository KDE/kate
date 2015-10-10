project (kateopenheader)
add_definitions(-DTRANSLATION_DOMAIN=\"kateopenheader\")

########### next target ###############
set(kateopenheaderplugin_PART_SRCS plugin_kateopenheader.cpp )

add_library(kateopenheaderplugin MODULE ${kateopenheaderplugin_PART_SRCS})

target_link_libraries(kateopenheaderplugin
  KF5::TextEditor
  KF5::I18n
  KF5::Parts)

########### install files ###############
kcoreaddons_desktop_to_json (kateopenheaderplugin kateopenheaderplugin.desktop)
install(TARGETS kateopenheaderplugin  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )

install( FILES ui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/kateopenheaderplugin )
