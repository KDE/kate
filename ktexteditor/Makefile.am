INCLUDES = -I$(top_srcdir)/kio -I$(top_srcdir) $(all_includes)

lib_LTLIBRARIES = libktexteditor.la

libktexteditor_la_SOURCES = ktexteditor.cpp editinterface.cpp editdcopinterface.cpp \
		editdcopinterface.skel clipboardinterface.cpp clipboarddcopinterface.cpp \
		clipboarddcopinterface.skel selectioninterface.cpp selectiondcopinterface.cpp \
		selectiondcopinterface.skel searchinterface.cpp searchdcopinterface.cpp \
		searchdcopinterface.skel codecompletioninterface.cpp

libktexteditor_la_LDFLAGS = $(all_libraries) $(KDE_RPATH)
libktexteditor_la_LIBADD = $(top_builddir)/kparts/libkparts.la $(top_builddir)/dcop/libDCOP.la

ktexteditorinclude_HEADERS = document.h view.h editinterface.h undointerface.h \
                             selectioninterface.h cursorinterface.h \
                             clipboardinterface.h popupmenuinterface.h \
                             viewcursorinterface.h searchinterface.h highlightinginterface.h

ktexteditorincludedir = $(includedir)/ktexteditor

METASOURCES = AUTO

servicetypedir = $(kde_servicetypesdir)
servicetype_DATA = ktexteditor.desktop
