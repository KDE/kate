INCLUDES = -I$(top_srcdir)/kio -I$(top_srcdir) -I$(top_srcdir)/interfaces -I$(top_builddir)/interfaces $(all_includes)

lib_LTLIBRARIES = libktexteditor.la

libktexteditor_la_SOURCES = ktexteditor.cpp \
    editinterface.cpp clipboardinterface.cpp  selectioninterface.cpp searchinterface.cpp \
    codecompletioninterface.cpp wordwrapinterface.cpp blockselectioninterface.cpp \
    configinterface.cpp cursorinterface.cpp  dynwordwrapinterface.cpp \
    printinterface.cpp highlightinginterface.cpp markinterface.cpp \
    popupmenuinterface.cpp undointerface.cpp viewcursorinterface.cpp \
    editdcopinterface.cpp editdcopinterface.skel clipboarddcopinterface.cpp \
    clipboarddcopinterface.skel selectiondcopinterface.cpp selectiondcopinterface.skel  \
    searchdcopinterface.cpp searchdcopinterface.skel markinterfaceextension.cpp \
    configinterfaceextension.cpp encodinginterface.cpp sessionconfiginterface.cpp \
    viewstatusmsginterface.cpp editorchooser_ui.ui editorchooser.cpp
    
libktexteditor_la_LDFLAGS = $(all_libraries) $(KDE_RPATH)
libktexteditor_la_LIBADD = $(LIB_KPARTS) $(top_builddir)/dcop/libDCOP.la

ktexteditorinclude_HEADERS = document.h view.h editor.h plugin.h editinterface.h undointerface.h \
                             selectioninterface.h cursorinterface.h \
                             clipboardinterface.h popupmenuinterface.h \
                             viewcursorinterface.h searchinterface.h highlightinginterface.h \
                             blockselectioninterface.h codecompletioninterface.h \
                             configinterface.h markinterface.h printinterface.h wordwrapinterface.h \
                             dynwordwrapinterface.h markinterfaceextension.h configinterfaceextension.h \
                             encodinginterface.h viewstatusmsginterface.h  sessionconfiginterface.h editorchooser.h

ktexteditorincludedir = $(includedir)/ktexteditor

METASOURCES = AUTO

servicetypedir = $(kde_servicetypesdir)
servicetype_DATA = ktexteditor.desktop ktexteditoreditor.desktop ktexteditorplugin.desktop

kcm_ktexteditor_DATA = kcm_ktexteditor.desktop
kcm_ktexteditordir = $(kde_datadir)/kcm_componentchooser
