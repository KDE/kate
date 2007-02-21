
add_subdirectory( actions ) 



########### install files ###############

install( FILES sessionchooser.png  DESTINATION  ${DATA_INSTALL_DIR}/kate/pics )

kde4_install_icons( ${ICON_INSTALL_DIR}  )

