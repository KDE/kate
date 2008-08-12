
########### next target ###############

set(ktexteditor_python-encoding_PART_SRCS python_encoding.cpp )


kde4_add_plugin(ktexteditor_python-encoding ${ktexteditor_python-encoding_PART_SRCS})

target_link_libraries(ktexteditor_python-encoding  ${KDE4_KIO_LIBS} ktexteditor kdeui kfile)

install(TARGETS  ktexteditor_python-encoding  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ktexteditor_python-encoding.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )




