


########### next target ###############

set(kwrite_KDEINIT_SRCS kwritemain.cpp )

kde4_automoc(kwrite ${kwrite_KDEINIT_SRCS})

kde4_add_kdeinit_executable( kwrite ${kwrite_KDEINIT_SRCS})

target_link_libraries(kdeinit_kwrite  ktexteditor ${KDE4_KPARTS_LIBS}  ${QT_QT3SUPPORT_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KIO_LIBS})

install(TARGETS kdeinit_kwrite  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kwrite kdeinit_kwrite )
install(TARGETS kwrite DESTINATION ${BIN_INSTALL_DIR})

########### install files ###############

install( FILES kwrite.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES kwriteui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kwrite )

