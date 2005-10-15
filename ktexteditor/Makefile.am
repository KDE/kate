INCLUDES = -I$(top_srcdir)/kio -I$(top_srcdir) -I$(top_srcdir)/interfaces -I$(top_builddir)/interfaces  -I$(top_srcdir)/kabc -I$(top_builddir)/kabc $(all_includes)

lib_LTLIBRARIES = libktexteditor.la

libktexteditor_la_SOURCES = ktexteditor.cpp range.cpp cursor.cpp \
	templateinterface.cpp 	editorchooser.cpp editorchooser_ui.ui attribute.cpp cursorfeedback.cpp \
	rangefeedback.cpp smartcursor.cpp smartrange.cpp
libktexteditor_la_LDFLAGS = -no-undefined $(all_libraries) $(KDE_RPATH)
libktexteditor_la_LIBADD = $(top_builddir)/interfaces/kdocument/libkdocument.la $(LIB_KPARTS)

ktexteditorinclude_HEADERS = factory.h editor.h document.h view.h plugin.h \
		cursor.h range.h configpage.h searchinterface.h highlightinginterface.h \
		codecompletioninterface.h markinterface.h sessionconfiginterface.h editorchooser.h 			 \
		texthintinterface.h variableinterface.h templateinterface.h 			 modificationinterface.h \
		commandinterface.h smartinterface.h attribute.h cursorfeedback.h rangefeedback.h \
	smartcursor.h smartrange.h

ktexteditorincludedir = $(includedir)/ktexteditor

kde_module_LTLIBRARIES = ktexteditorkabcbridge.la
ktexteditorkabcbridge_la_SOURCES=ktexteditorkabcbridge.cpp
ktexteditorkabcbridge_la_LIBADD = $(LIB_KDECORE) $(top_builddir)/kabc/libkabc.la

ktexteditorkabcbridge_la_LDFLAGS = -module $(all_libraries) $(KDE_PLUGIN)

METASOURCES = AUTO

servicetypedir = $(kde_servicetypesdir)
servicetype_DATA = ktexteditor.desktop ktexteditorplugin.desktop

kcm_ktexteditor_DATA = kcm_ktexteditor.desktop
kcm_ktexteditordir = $(kde_datadir)/kcm_componentchooser

templateinterface.lo: $(top_builddir)/kabc/addressee.h

DOXYGEN_REFERENCES = kdecore kdeui kparts
include ../../admin/Doxyfile.am

