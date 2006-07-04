


########### next target ###############

set(kwrite_KDEINIT_SRCS kwritemain.cpp )

kde4_automoc(${kwrite_KDEINIT_SRCS})

kde4_add_kdeinit_executable( kwrite ${kwrite_KDEINIT_SRCS})

target_link_libraries(kdeinit_kwrite  ${KDE4_KDECORE_LIBS} ktexteditor kparts  ${QT_QT3SUPPORT_LIBRARY} kdeui ${QT_QTGUI_LIBRARY} kio)

install(TARGETS kdeinit_kwrite  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kwrite kdeinit_kwrite )
install(TARGETS kwrite DESTINATION bin)

########### install files ###############

install( FILES kwrite.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES kwriteui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kwrite )




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
