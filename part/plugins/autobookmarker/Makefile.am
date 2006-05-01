INCLUDES    = -I$(top_srcdir)/interfaces $(all_includes)
METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = ktexteditor_autobookmarker.la

ktexteditor_autobookmarker_la_SOURCES = autobookmarker.cpp
ktexteditor_autobookmarker_la_LIBADD = $(top_builddir)/interfaces/ktexteditor/libktexteditor.la
ktexteditor_autobookmarker_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

kde_services_DATA = ktexteditor_autobookmarker.desktop

confdir=$(kde_confdir)
conf_DATA = ktexteditor_autobookmarkerrc

