INCLUDES    = -I$(top_srcdir)/interfaces $(all_includes)
METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = ktexteditor_isearch.la

ktexteditor_isearch_la_SOURCES = ISearchPlugin.cpp
ktexteditor_isearch_la_LIBADD = $(top_builddir)/interfaces/ktexteditor/libktexteditor.la
ktexteditor_isearch_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
          
datadir = $(kde_datadir)/ktexteditor_isearch
data_DATA = ktexteditor_isearchui.rc

kde_services_DATA = ktexteditor_isearch.desktop

messages: rc.cpp
	$(XGETTEXT) *.cpp *.h -o $(podir)/ktexteditor_isearch.pot

