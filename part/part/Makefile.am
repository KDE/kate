kde_module_LTLIBRARIES = libkatepart.la

libkatepart_la_SOURCES = katesearch.cpp katebuffer.cpp katecmds.cpp \
  kateundo.cpp katecursor.cpp katedialogs.cpp katedocument.cpp \
  katefactory.cpp katehighlight.cpp katesyntaxdocument.cpp \
  katetextline.cpp kateview.cpp kateconfig.cpp kateviewhelpers.cpp \
  katecodecompletion.cpp katedocumenthelpers.cpp \
  katecodefoldinghelpers.cpp kateviewinternal.cpp katebookmarks.cpp \
  kateprinter.cpp katefont.cpp katelinerange.cpp katesupercursor.cpp \
  katearbitraryhighlight.cpp katerenderer.cpp kateattribute.cpp \
  kateautoindent.cpp katefiletype.cpp kateschema.cpp katedocument.skel \
  katetemplatehandler.cpp katejscript.cpp

libkatepart_la_LIBADD = ../interfaces/libkatepartinterfaces.la $(top_builddir)/kdeprint/libkdeprint.la $(top_builddir)/kutils/libkutils.la $(top_builddir)/kjs/libkjs.la

libkatepart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)

INCLUDES= -I../interfaces -I$(top_srcdir) -I$(top_srcdir)/kdeprint -I$(top_srcdir)/interfaces  -I$(top_srcdir)/interfaces/kregexpeditor -I$(top_srcdir)/kdefx -I$(top_srcdir)/kutils -I$(top_builddir)/kjs $(all_includes)

METASOURCES = AUTO

LUT_FILES = katejscript.lut.h

CREATE_HASH_TABLE = $(top_srcdir)/kjs/create_hash_table

katejscript.lut.h : $(srcdir)/katejscript.cpp $(CREATE_HASH_TABLE)
	$(PERL) $(CREATE_HASH_TABLE) $(srcdir)/katejscript.cpp > $@
katejscript.lo: katejscript.lut.h

CLEANFILES = $(LUT_FILES)
