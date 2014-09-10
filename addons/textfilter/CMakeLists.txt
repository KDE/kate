########### next target ###############
set(katetextfilterplugin_PART_SRCS plugin_katetextfilter.cpp )

set(katetextfilterplugin_PART_UI textfilterwidget.ui)

kde4_add_ui_files(katetextfilterplugin_PART_SRCS ${katetextfilterplugin_PART_UI} )
kde4_add_plugin(katetextfilterplugin ${katetextfilterplugin_PART_SRCS})

target_link_libraries(katetextfilterplugin  ${KDE4_KDEUI_LIBS} ktexteditor kateinterfaces )

install(TARGETS katetextfilterplugin  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )

########### install files ###############
install( FILES ui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/kate/plugins/katetextfilter )
