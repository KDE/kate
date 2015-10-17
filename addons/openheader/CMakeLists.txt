project (kateopenheader)
add_definitions(-DTRANSLATION_DOMAIN=\"kateopenheader\")

########### next target ###############
set(kateopenheaderplugin_PART_SRCS plugin_kateopenheader.cpp )

# resource for ui file and stuff
qt5_add_resources(kateopenheaderplugin_PART_SRCS plugin.qrc)

add_library(kateopenheaderplugin MODULE ${kateopenheaderplugin_PART_SRCS})
kcoreaddons_desktop_to_json (kateopenheaderplugin kateopenheaderplugin.desktop)
target_link_libraries(kateopenheaderplugin
  KF5::TextEditor
  KF5::I18n
  KF5::Parts)

install(TARGETS kateopenheaderplugin  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )
