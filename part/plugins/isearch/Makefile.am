
INCLUDES= -I$(top_srcdir) -I$(top_srcdir)/libltdl -I$(top_srcdir)/kfile -I$(top_srcdir)/kio $(all_includes)
LDFLAGS	= $(all_libraries)

# These are not really libraries, but modules dynamically opened.
# So they should be installed in kde_module_dir, which is usually $prefix/lib/kde2
kde_module_LTLIBRARIES = ktexteditorviewpluginisearch.la

ktexteditorviewpluginisearch_la_SOURCES = plugin_isearch.cpp
ktexteditorviewpluginisearch_la_LIBADD = -lktexteditor -lkparts $(LIB_KDEUI)
ktexteditorviewpluginisearch_la_LDFLAGS = -module $(KDE_PLUGIN)
          
rcisearchdir = $(kde_datadir)/ktexteditorviewpluginisearch
rcisearch_DATA = isearchui.rc

kde_services_DATA = ktexteditorviewpluginisearch.desktop

METASOURCES = AUTO

