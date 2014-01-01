
########### next target ###############

set(ktexteditor_hlselection_PART_SRCS hlselectionplugin.cpp )

add_library(ktexteditor_hlselection MODULE ${ktexteditor_hlselection_PART_SRCS})

target_link_libraries(ktexteditor_hlselection ktexteditor KF5::I18n)

install(TARGETS  ktexteditor_hlselection  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

kservice_desktop_to_json(ktexteditor_hlselection.desktop)
install( FILES ktexteditor_hlselection.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

# Dominik: right now, we do not have any actions, so do not insall ui.rc file
# install( FILES ktexteditor_hlselectionui.rc  DESTINATION  ${DATA_INSTALL_DIR}/ktexteditor_hlselection )
