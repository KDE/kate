kde4_header()

include_directories( ${CMAKE_SOURCE_DIR}/interfaces ${KDE4_INCLUDE_DIR} ${QT_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(ktexteditor_autobookmarker_PART_SRCS
autobookmarker.cpp
)

kde4_automoc(${ktexteditor_autobookmarker_PART_SRCS})

kde4_add_plugin(ktexteditor_autobookmarker ${ktexteditor_autobookmarker_PART_SRCS})

target_link_libraries(ktexteditor_autobookmarker  ${QT_AND_KDECORE_LIBS} ktexteditor )

install_targets(/lib/kde4 ktexteditor_autobookmarker )


########### install files ###############

install_files( ${KDE4_CONFIG_DIR} FILES ktexteditor_autobookmarkerrc )
install_files( ${KDE4_SERVICES_DIR} FILES ktexteditor_autobookmarker.desktop )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES    = -I$(top_srcdir)/interfaces $(all_includes)
#METASOURCES = AUTO
#
## Install this plugin in the KDE modules directory
#kde_module_LTLIBRARIES = ktexteditor_autobookmarker.la
#
#ktexteditor_autobookmarker_la_SOURCES = autobookmarker.cpp
#ktexteditor_autobookmarker_la_LIBADD = $(top_builddir)/interfaces/ktexteditor/libktexteditor.la
#ktexteditor_autobookmarker_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
#kde_services_DATA = ktexteditor_autobookmarker.desktop
#
#confdir=$(kde_confdir)
#conf_DATA = ktexteditor_autobookmarkerrc
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp *.h -o $(podir)/ktexteditor_autobookmarker.pot
