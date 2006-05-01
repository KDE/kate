kde4_header()

########### next target ###############

set(ktexteditor_insertfile_PART_SRCS insertfileplugin.cpp )

kde4_automoc(${ktexteditor_insertfile_PART_SRCS})

kde4_add_plugin(ktexteditor_insertfile ${ktexteditor_insertfile_PART_SRCS})

target_link_libraries(ktexteditor_insertfile  ${KDE4_KDECORE_LIBS} ktexteditor )

kde4_install_libtool_file(${PLUGIN_INSTALL_DIR} ktexteditor_insertfile )
install_targets(${PLUGIN_INSTALL_DIR}  ktexteditor_insertfile )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES ktexteditor_insertfile.desktop )
install_files( ${DATA_INSTALL_DIR}/ktexteditor_insertfile FILES ktexteditor_insertfileui.rc )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES    = -I$(top_srcdir)/interfaces $(all_includes)
#METASOURCES = AUTO
#
## Install this plugin in the KDE modules directory
#kde_module_LTLIBRARIES = ktexteditor_insertfile.la
#
#ktexteditor_insertfile_la_SOURCES = insertfileplugin.cpp
#ktexteditor_insertfile_la_LIBADD = $(top_builddir)/interfaces/ktexteditor/libktexteditor.la
#ktexteditor_insertfile_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#          
#insertfiledatadir = $(kde_datadir)/ktexteditor_insertfile
#insertfiledata_DATA = ktexteditor_insertfileui.rc
#
#kde_services_DATA = ktexteditor_insertfile.desktop
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp *.h -o $(podir)/ktexteditor_insertfile.pot
