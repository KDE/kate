kde4_header()



########### next target ###############

set(kwrite_KDEINIT_SRCS kwritemain.cpp )

kde4_automoc(${kwrite_KDEINIT_SRCS})

kde4_add_kdeinit_executable( kwrite ${kwrite_KDEINIT_SRCS})

target_link_libraries(kdeinit_kwrite  ${KDE4_KDECORE_LIBS} ktexteditor kparts  ${QT_QT3SUPPORT_LIBRARY} kdeui ${QT_QTGUI_LIBRARY} DCOP kio)

install_targets(${LIB_INSTALL_DIR} kdeinit_kwrite )

target_link_libraries( kwrite kdeinit_kwrite )
install(TARGETS kwrite DESTINATION bin)

########### install files ###############

install_files( ${XDG_APPS_DIR} FILES kwrite.desktop )
install_files( ${DATA_INSTALL_DIR}/kwrite FILES kwriteui.rc )

kde4_footer()



#original Makefile.am contents follow:

#bin_PROGRAMS =
#kdeinit_LTLIBRARIES = kwrite.la
#
#kwrite_la_SOURCES = kwritemain.cpp
#kwrite_la_LIBADD   = -lktexteditor
#kwrite_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -avoid-version
#
#METASOURCES = AUTO
#
#INCLUDES= $(all_includes)
#
#
#xdg_apps_DATA = kwrite.desktop
#
#rckwritedir = $(kde_datadir)/kwrite
#rckwrite_DATA = kwriteui.rc
