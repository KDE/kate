METASOURCES = application.moc docmanager.moc document.moc mainwindow.moc plugin.moc view.moc viewmanager.moc

lib_LTLIBRARIES = libkateinterfaces.la

libkateinterfaces_la_SOURCES = interfaces.cpp
libkateinterfaces_la_LIBADD   = -lktexteditor
libkateinterfaces_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(KDE_PLUGIN)  -module -avoid-version

kateinclude_HEADERS = application.h docmanager.h document.h mainwindow.h plugin.h view.h viewmanager.h
kateincludedir = $(includedir)/kate

INCLUDES= $(all_includes)
