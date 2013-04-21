install( FILES katepartui.rc katepartsimpleui.rc  DESTINATION  ${DATA_INSTALL_DIR}/katepart )
install( FILES katepart.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES katemoderc kateschemarc katesyntaxhighlightingrc DESTINATION  ${CONFIG_INSTALL_DIR} )
install( FILES ktexteditor_codesnippets_core.knsrc  DESTINATION  ${CONFIG_INSTALL_DIR} )
add_subdirectory (actions)

