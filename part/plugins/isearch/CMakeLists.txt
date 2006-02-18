kde4_header()

include_directories( ${CMAKE_SOURCE_DIR}/interfaces ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(ktexteditor_isearch_PART_SRCS
ISearchPlugin.cpp
)

kde4_automoc(${ktexteditor_isearch_PART_SRCS})

kde4_add_plugin(ktexteditor_isearch ${ktexteditor_isearch_PART_SRCS})


target_link_libraries(ktexteditor_isearch  ${QT_AND_KDECORE_LIBS} ktexteditor )

kde4_install_libtool_file(${PLUGIN_INSTALL_DIR} ktexteditor_isearch )
install_targets(${PLUGIN_INSTALL_DIR} ktexteditor_isearch )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES ktexteditor_isearch.desktop )
install_files( ${DATA_INSTALL_DIR}/ktexteditor_isearch FILES ktexteditor_isearchui.rc )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES    = -I$(top_srcdir)/interfaces $(all_includes)
#METASOURCES = AUTO
#
## Install this plugin in the KDE modules directory
#kde_module_LTLIBRARIES = ktexteditor_isearch.la
#
#ktexteditor_isearch_la_SOURCES = ISearchPlugin.cpp
#ktexteditor_isearch_la_LIBADD = $(top_builddir)/interfaces/ktexteditor/libktexteditor.la
#ktexteditor_isearch_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#          
#isearchdatadir = $(kde_datadir)/ktexteditor_isearch
#isearchdata_DATA = ktexteditor_isearchui.rc
#
#kde_services_DATA = ktexteditor_isearch.desktop
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp *.h -o $(podir)/ktexteditor_isearch.pot
#
