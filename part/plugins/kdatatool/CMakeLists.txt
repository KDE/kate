kde4_header()

include_directories( ${CMAKE_SOURCE_DIR}/interfaces ${CMAKE_SOURCE_DIR}/kdefx ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(ktexteditor_kdatatool_PART_SRCS
kate_kdatatool.cpp
)

kde4_automoc(${ktexteditor_kdatatool_PART_SRCS})

kde4_add_plugin(ktexteditor_kdatatool ${ktexteditor_kdatatool_PART_SRCS})

target_link_libraries(ktexteditor_kdatatool  ${QT_AND_KDECORE_LIBS} ktexteditor )

kde4_install_libtool_file(${PLUGIN_INSTALL_DIR} ktexteditor_kdatatool )
install_targets(${PLUGIN_INSTALL_DIR} ktexteditor_kdatatool )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/ktexteditor_kdatatool FILES ktexteditor_kdatatoolui.rc )
install_files( ${SERVICES_INSTALL_DIR} FILES ktexteditor_kdatatool.desktop )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES    = -I$(top_srcdir)/interfaces -I$(top_srcdir)/kdefx $(all_includes)
#METASOURCES = AUTO
#
## Install this plugin in the KDE modules directory
#kde_module_LTLIBRARIES = ktexteditor_kdatatool.la
#
#ktexteditor_kdatatool_la_SOURCES = kate_kdatatool.cpp
#ktexteditor_kdatatool_la_LIBADD = $(top_builddir)/interfaces/ktexteditor/libktexteditor.la
#ktexteditor_kdatatool_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
#kdatatooldatadir = $(kde_datadir)/ktexteditor_kdatatool
#kdatatooldata_DATA = ktexteditor_kdatatoolui.rc
#
#kde_services_DATA = ktexteditor_kdatatool.desktop
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp *.h -o $(podir)/ktexteditor_kdatatool.pot
#
#
