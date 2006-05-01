INCLUDES    = -I$(top_srcdir)/interfaces $(all_includes)
METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = ktexteditor_docwordcompletion.la

ktexteditor_docwordcompletion_la_SOURCES = docwordcompletion.cpp
ktexteditor_docwordcompletion_la_LIBADD = $(top_builddir)/interfaces/ktexteditor/libktexteditor.la
ktexteditor_docwordcompletion_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

docwordcompletiondatadir = $(kde_datadir)/ktexteditor_docwordcompletion
docwordcompletiondata_DATA = docwordcompletionui.rc

kde_services_DATA = ktexteditor_docwordcompletion.desktop

