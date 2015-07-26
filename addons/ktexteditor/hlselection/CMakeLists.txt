
########### next target ###############

set(ktexteditor_hlselection_PART_SRCS hlselectionplugin.cpp )

kde4_add_plugin(ktexteditor_hlselection ${ktexteditor_hlselection_PART_SRCS})

target_link_libraries(ktexteditor_hlselection  ${KDE4_KIO_LIBS} ${KDE4_KTEXTEDITOR_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KFILE_LIBS})

install(TARGETS  ktexteditor_hlselection  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ktexteditor_hlselection.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

# Dominik: right now, we do not have any actions, so do not insall ui.rc file
# install( FILES ktexteditor_hlselectionui.rc  DESTINATION  ${DATA_INSTALL_DIR}/ktexteditor_hlselection )
