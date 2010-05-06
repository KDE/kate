
include_directories( ${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

########### next target ###############

set(ktexteditor_insanehtml_le_PART_SRCS
   insanehtmlplugin_le.cpp)

#kde4_add_ui_files(ktexteditor_insanehtml_le_PART_SRCS insanehtml.ui )

kde4_add_plugin(ktexteditor_insanehtml_le ${ktexteditor_insanehtml_le_PART_SRCS})

target_link_libraries(ktexteditor_insanehtml_le  ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ktexteditor)

install(TARGETS ktexteditor_insanehtml_le DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  data/insanehtml_le_ui.rc DESTINATION ${DATA_INSTALL_DIR}/ktexteditor_insanehtml_le )
install( FILES  data/xhtml.cfg DESTINATION ${DATA_INSTALL_DIR}/ktexteditor_insanehtml_le )
install( FILES  data/ktexteditor_insanehtml_le.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


