# desktop files and co
install( PROGRAMS org.kde.kate.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES org.kde.kate.appdata.xml DESTINATION  ${CMAKE_INSTALL_METAINFODIR} )
install( FILES katerc  DESTINATION  ${CONFIG_INSTALL_DIR} )
install( FILES kateui.rc DESTINATION  ${KXMLGUI_INSTALL_DIR}/kate )
