



########### next target ###############

set(katesnippetsplugin_PART_SRCS
   plugin_katesnippets.cpp
   katesnippetswidget.h
   katesnippetswidget.cpp
   katesnippet.cpp
   KateSnippetsWidgetBase.ui.h )


kde4_add_ui3_files(katesnippetsplugin_PART_SRCS KateSnippetsWidgetBase.ui )

kde4_add_plugin(katesnippetsplugin ${katesnippetsplugin_PART_SRCS})



target_link_libraries(katesnippetsplugin  ${KDE4_KDEUI_LIBS} ${KDE4_KTEXTEDITOR_LIBS} kateinterfaces ${QT_QT3SUPPORT_LIBRARY})

install(TARGETS katesnippetsplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES plugin_katesnippets.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katesnippets )
install( FILES katesnippets.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )


