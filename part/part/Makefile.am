kde_module_LTLIBRARIES = libkatepart.la

libkatepart_la_SOURCES = katesearch.cpp katebuffer.cpp katecmds.cpp \
			 kateundo.cpp katehledit_attrib_skel.ui katecursor.cpp \
			 katedialogs.cpp katedocument.cpp katefactory.cpp katehighlight.cpp \
			 katesyntaxdocument.cpp katetextline.cpp kateview.cpp \
			 kateviewdialog.cpp \
			 katehighlightdownload.cpp katefiledialog.cpp kateconfig.cpp \
			 kateiconborder.cpp katecodecompletion_arghint.cpp \
			 katecodecompletion.cpp hlparamedit.cpp kateexportaction.cpp \
			 attribeditor.cpp katecodefoldinghelpers.cpp kateviewinternal.cpp \
			 katebookmarks.cpp katebrowserextension.cpp kateprintsettings.cpp \
			 katefont.cpp katelinerange.cpp katesupercursor.cpp katearbitraryhighlight.cpp \
			 katerenderer.cpp kateattribute.cpp katecmdline.cpp katecorecommands.cpp \
			 kateautoindent.cpp katefiletype.cpp kateschema.cpp katepythonindent.cpp

libkatepart_la_LIBADD   = ../interfaces/libkatepartinterfaces.la $(top_builddir)/kdeprint/libkdeprint.la $(top_builddir)/kutils/libkutils.la
libkatepart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)

INCLUDES= -I../interfaces -I$(top_srcdir) -I$(top_srcdir)/kdeprint -I$(top_srcdir)/interfaces  -I$(top_srcdir)/interfaces/kregexpeditor -I$(top_srcdir)/kdefx -I$(top_srcdir)/kutils $(all_includes)

METASOURCES = AUTO
