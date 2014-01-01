# desktop files and co
install( PROGRAMS kate.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES katerc  DESTINATION  ${CONFIG_INSTALL_DIR} )
install( FILES tips kateui.rc DESTINATION  ${DATA_INSTALL_DIR}/kate )

# kate application icons
add_subdirectory (icons)
