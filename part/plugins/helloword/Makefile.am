
INCLUDES= -I$(top_srcdir) -I$(top_srcdir)/libltdl -I$(top_srcdir)/kfile -I$(top_srcdir)/kio $(all_includes)
LDFLAGS	= $(all_libraries)

# These are not really libraries, but modules dynamically opened.
# So they should be installed in kde_module_dir, which is usually $prefix/lib/kde2
kde_module_LTLIBRARIES = ktexteditorviewpluginhelloworld.la

ktexteditorviewpluginhelloworld_la_SOURCES = plugin_helloworld.cpp
ktexteditorviewpluginhelloworld_la_LIBADD = -lktexteditor -lkparts $(LIB_KDEUI)
ktexteditorviewpluginhelloworld_la_LDFLAGS = -module $(KDE_PLUGIN)

kde_services_DATA = ktexteditorviewpluginhelloworld.desktop

METASOURCES = AUTO

