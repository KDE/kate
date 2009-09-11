

########### next target ###############

set(katesnippets_tngplugin_PART_SRCS jowennsnippets.cpp repository.cpp completionmodel.cpp selector.cpp)

set(katesnippets_tngplugin_PART_UI
  snippet_repository.ui
  snippet_selector.ui
)
kde4_add_ui_files(katesnippets_tngplugin_PART_SRCS ${katesnippets_tngplugin_PART_UI} )


kde4_add_plugin(katesnippets_tngplugin ${katesnippets_tngplugin_PART_SRCS})


target_link_libraries(katesnippets_tngplugin  ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS} ${KDE4_KTEXTEDITOR_LIBS} kateinterfaces)

install(TARGETS katesnippets_tngplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katesnippets_tng )

file(GLOB SNIPPETS "${CMAKE_CURRENT_SOURCE_DIR}/data/*.xml")
install( FILES ${SNIPPETS} DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katesnippets_tng/data )

install( FILES katesnippets_tngplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

