kde_module_LTLIBRARIES = libkatepart.la

libkatepart_la_SOURCES = katesearch.cpp katebuffer.cpp katecmds.cpp \
  kateundo.cpp katecursor.cpp katedialogs.cpp katedocument.cpp \
  katefactory.cpp katehighlight.cpp katesyntaxdocument.cpp \
  katetextline.cpp kateview.cpp katehighlightdownload.cpp kateconfig.cpp \
  kateiconborder.cpp katecodecompletion_arghint.cpp \
  katecodecompletion.cpp kateexportaction.cpp \
  katecodefoldinghelpers.cpp kateviewinternal.cpp \
  katebookmarks.cpp katebrowserextension.cpp kateprintsettings.cpp \
  katefont.cpp katelinerange.cpp katesupercursor.cpp katearbitraryhighlight.cpp \
  katerenderer.cpp kateattribute.cpp katecmdline.cpp \
  kateautoindent.cpp katefiletype.cpp kateschema.cpp katepythonindent.cpp katedocument.skel

libkatepart_la_LIBADD = ../interfaces/libkatepartinterfaces.la $(top_builddir)/kdeprint/libkdeprint.la $(top_builddir)/kutils/libkutils.la

libkatepart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)

INCLUDES= -I../interfaces -I$(top_srcdir) -I$(top_srcdir)/kdeprint -I$(top_srcdir)/interfaces  -I$(top_srcdir)/interfaces/kregexpeditor -I$(top_srcdir)/kdefx -I$(top_srcdir)/kutils $(all_includes)

METASOURCES = AUTO
