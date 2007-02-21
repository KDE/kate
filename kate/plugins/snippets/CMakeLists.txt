



########### next target ###############

set(katesnippetsplugin_PART_SRCS 
   plugin_katesnippets.cpp 
   cwidgetsnippets.h 
   cwidgetsnippets.cpp 
   csnippet.h 
   csnippet.cpp 
   CWidgetSnippetsBase.ui.h )

kde4_automoc(${katesnippetsplugin_PART_SRCS})

kde4_add_ui3_files(katesnippetsplugin_PART_SRCS CWidgetSnippetsBase.ui )

kde4_add_plugin(katesnippetsplugin ${katesnippetsplugin_PART_SRCS})



target_link_libraries(katesnippetsplugin  ${KDE4_KDEUI_LIBS} ktexteditor kateinterfaces )

install(TARGETS katesnippetsplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES plugin_katesnippets.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katesnippets )
install( FILES katesnippets.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )


