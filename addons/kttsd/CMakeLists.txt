add_definitions(-DTRANSLATION_DOMAIN=\"kate_kttsd\")
set(kate_kttsd_PART_SRCS katekttsd.cpp )

kde4_add_plugin(kate_kttsd ${kate_kttsd_PART_SRCS})

target_link_libraries(kate_kttsd ${KDE4_KDECORE_LIBS} ktexteditor ${KDE4_KPARTS_LIBS} kateinterfaces )


install(TARGETS kate_kttsd  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )


########### install files ###############

install( FILES ui.rc DESTINATION  ${KXMLGUI_INSTALL_DIR}/kate/plugins/kate_kttsd)
