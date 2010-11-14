########### next target ###############

set(katetabbarextensionplugin_PART_SRCS
   plugin_katetabbarextension.cpp
   ktinytabbutton.cpp
   ktinytabbar.cpp
   ktinytabbarconfigdialog.cpp
   ktinytabbarconfigpage.cpp )

set(katetabbarextensionplugin_PART_UI
    tabbarconfigwidget.ui
)

kde4_add_ui_files(katetabbarextensionplugin_PART_SRCS ${katetabbarextensionplugin_PART_UI} )

kde4_add_plugin(katetabbarextensionplugin ${katetabbarextensionplugin_PART_SRCS})

target_link_libraries(katetabbarextensionplugin  ${KDE4_KDEUI_LIBS} ${QT_QT3SUPPORT_LIBRARY} ${KDE4_KPARTS_LIBS} kateinterfaces )

install(TARGETS katetabbarextensionplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katetabbarextension )
install( FILES katetabbarextension.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
