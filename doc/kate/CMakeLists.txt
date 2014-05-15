########### install files ###############

kdoctools_create_handbook (index.docbook INSTALL_DESTINATION ${HTML_INSTALL_DIR}/en SUBDIR kate)
kdoctools_create_manpage (man-kate.1.docbook 1 INSTALL_DESTINATION ${MAN_INSTALL_DIR})
