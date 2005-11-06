kde_module_LTLIBRARIES = katepart.la

katepart_la_SOURCES = katesearch.cpp katebuffer.cpp katecmds.cpp kateundo.cpp \
	katecursor.cpp katedialogs.cpp katedocument.cpp katefactory.cpp katehighlight.cpp \
	katesyntaxdocument.cpp katetextline.cpp kateview.cpp kateconfig.cpp kateviewhelpers.cpp \
	katecodecompletion.cpp katedocumenthelpers.cpp katecodefoldinghelpers.cpp kateviewinternal.cpp \
	katebookmarks.cpp katefont.cpp katelinerange.cpp katesmartcursor.cpp katerenderer.cpp \
	kateautoindent.cpp katefiletype.cpp kateschema.cpp katedocument.skel \
	katetemplatehandler.cpp katejscript.cpp katespell.cpp kateprinter.cpp \
	kateindentscriptabstracts.cpp kateluaindentscript.cpp katerange.cpp katesmartrange.cpp kateglobal.cpp \
	katecmd.cpp katelayoutcache.cpp katetextlayout.cpp katesmartmanager.cpp \
	kateedit.cpp katesmartcursornotifier.cpp katerenderrange.cpp katesmartregion.cpp \
	katedynamicanimation.cpp kateextendedattribute.cpp

katepart_la_LIBADD = $(top_builddir)/interfaces/ktexteditor/libktexteditor.la $(top_builddir)/kdeprint/libkdeprint.la $(top_builddir)/kutils/libkutils.la $(top_builddir)/kjs/libkjs.la $(LUA_LIBS)

katepart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/kdeprint \
	-I$(top_srcdir)/interfaces -I$(top_srcdir)/interfaces/kregexpeditor -I$(top_srcdir)/kdefx \
	-I$(top_srcdir)/kutils -I$(top_builddir)/kjs $(LUA_INCLUDES) $(all_includes)

METASOURCES = AUTO

LUT_FILES = katejscript.lut.h

CREATE_HASH_TABLE = $(top_srcdir)/kjs/create_hash_table

katejscript.lut.h : $(srcdir)/katejscript.cpp $(CREATE_HASH_TABLE)
	$(PERL) $(CREATE_HASH_TABLE) $(srcdir)/katejscript.cpp > $@
katejscript.lo: katejscript.lut.h

CLEANFILES = $(LUT_FILES)
noinst_HEADERS = katerenderrange.h katesmartregion.h katedynamicanimation.h \
	kateextendedattribute.h
