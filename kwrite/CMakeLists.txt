set(kwrite_KDEINIT_SRCS kwritemain.cpp)

kde4_add_kdeinit_executable(kwrite ${kwrite_KDEINIT_SRCS})
target_link_libraries(kdeinit_kwrite ${KDE4_KTEXTEDITOR_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KPARTS_LIBS})

install(TARGETS kwrite         ${INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS kdeinit_kwrite ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES kwrite.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES kwriteui.rc DESTINATION ${DATA_INSTALL_DIR}/kwrite)
