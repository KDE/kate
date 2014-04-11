# desktop files and co
install( PROGRAMS org.kde.kate.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES katerc  DESTINATION  ${CONFIG_INSTALL_DIR} )
install( FILES kateui.rc DESTINATION  ${DATA_INSTALL_DIR}/kate )
