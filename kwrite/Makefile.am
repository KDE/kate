bin_PROGRAMS =
kdeinit_LTLIBRARIES = kwrite.la

kwrite_la_SOURCES = kwritemain.cpp
kwrite_la_LIBADD   = -lktexteditor
kwrite_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -avoid-version

METASOURCES = AUTO

INCLUDES= $(all_includes)


xdg_apps_DATA = kwrite.desktop

rckwritedir = $(kde_datadir)/kwrite
rckwrite_DATA = kwriteui.rc
