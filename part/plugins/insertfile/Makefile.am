INCLUDES    = -I$(top_srcdir)/interfaces $(all_includes)
METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = ktexteditor_insertfile.la

ktexteditor_insertfile_la_SOURCES = insertfileplugin.cpp
ktexteditor_insertfile_la_LIBADD = $(top_builddir)/interfaces/ktexteditor/libktexteditor.la
ktexteditor_insertfile_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
          
datadir = $(kde_datadir)/ktexteditor_insertfile
data_DATA = ktexteditor_insertfileui.rc

kde_services_DATA = ktexteditor_insertfile.desktop

messages:
	$(XGETTEXT) *.cpp *.h -o $(podir)/ktexteditor_insertfile.pot
