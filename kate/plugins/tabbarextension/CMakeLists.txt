########### next target ###############

set(katetabbarextensionplugin_PART_SRCS 
   plugin_katetabbarextension.cpp 
   ktinytabbutton.cpp 
   ktinytabbar.cpp 
   ktinytabbarconfigdialog.cpp 
   ktinytabbarconfigpage.cpp )

kde4_automoc(${katetabbarextensionplugin_PART_SRCS})

kde4_add_plugin(katetabbarextensionplugin ${katetabbarextensionplugin_PART_SRCS})

target_link_libraries(katetabbarextensionplugin  ${KDE4_KDEUI_LIBS} kateinterfaces )

install(TARGETS katetabbarextensionplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katetabbarextension )
install( FILES katetabbarextension.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
