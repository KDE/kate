INCLUDES    = $(all_includes)
METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = ktexteditor_isearch.la

ktexteditor_isearch_la_SOURCES = ISearchPlugin.cpp
ktexteditor_isearch_la_LIBADD = -lktexteditor -lkparts $(LIB_KDEUI)
ktexteditor_isearch_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
          
datadir = $(kde_datadir)/ktexteditor_isearch
data_DATA = ktexteditor_isearchui.rc

kde_services_DATA = ktexteditor_isearch.desktop

messages:
	$(XGETTEXT) *.cpp *.h -o $(podir)/ktexteditor_isearch.pot

