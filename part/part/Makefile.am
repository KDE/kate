kde_module_LTLIBRARIES = libkatepart.la

libkatepart_la_SOURCES = katesearch.cpp katebuffer.cpp katecmd.cpp katecmds.cpp \
			 kateundo.cpp katehledit_attrib_skel.ui katecursor.cpp \
			 katedialogs.cpp katedocument.cpp katefactory.cpp katehighlight.cpp \
			 katesyntaxdocument.cpp katetextline.cpp kateview.cpp \
			 kateviewdialog.cpp kateviewhighlightaction.cpp \
			 katehighlightdownload.cpp katefiledialog.cpp \
			 kateiconborder.cpp katecodecompletion_arghint.cpp \
			 katecodecompletion.cpp hlparamedit.cpp kateexportaction.cpp \
			 attribeditor.cpp katecodefoldinghelpers.cpp kateviewinternal.cpp \
			 katebookmarks.cpp katebrowserextension.cpp kateprintsettings.cpp \
			 katefont.cpp

libkatepart_la_LIBADD   = ../interfaces/libkatepartinterfaces.la $(top_builddir)/kdeprint/libkdeprint.la $(top_builddir)/kutils/libkutils.la
libkatepart_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(KDE_PLUGIN) -module -avoid-version

INCLUDES= -I../interfaces -I$(top_srcdir) -I$(top_srcdir)/kdeprint -I$(top_srcdir)/interfaces  -I$(top_srcdir)/interfaces/kregexpeditor -I$(top_srcdir)/kdefx $(all_includes)

METASOURCES = AUTO
