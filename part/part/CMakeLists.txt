kde4_header()

include_directories( ${CMAKE_SOURCE_DIR}/kdeprint ${CMAKE_SOURCE_DIR}/interfaces ${CMAKE_SOURCE_DIR}/interfaces/kregexpeditor ${CMAKE_SOURCE_DIR}/kdefx ${CMAKE_SOURCE_DIR}/kutils ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############
#CREATE_HASH_TABLE = $(top_srcdir)/kjs/create_hash_table
#
#katejscript.lut.h : $(srcdir)/katejscript.cpp $(CREATE_HASH_TABLE)
#	$(PERL) $(CREATE_HASH_TABLE) $(srcdir)/katejscript.cpp > $@
#katejscript.lo: katejscript.lut.h

set(CREATE_HASH_TABLE ${CMAKE_SOURCE_DIR}/kjs/create_hash_table )

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/katejscript.lut.h
   COMMAND ${PERL_EXECUTABLE}
   ARGS ${CREATE_HASH_TABLE} ${CMAKE_CURRENT_SOURCE_DIR}/katejscript.cpp >
   ${CMAKE_CURRENT_BINARY_DIR}/katejscript.lut.h
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/katejscript.cpp
)

macro_add_file_dependencies(${CMAKE_CURRENT_SOURCE_DIR}/katejscript.cpp ${CMAKE_CURRENT_BINARY_DIR}/katejscript.lut.h )


set(katepart_PART_SRCS
katesearch.cpp
katebuffer.cpp
katecmds.cpp
kateundo.cpp
katecursor.cpp
katedialogs.cpp
katedocument.cpp
katefactory.cpp
katehighlight.cpp
katesyntaxdocument.cpp
katetextline.cpp
kateview.cpp
kateconfig.cpp
kateviewhelpers.cpp
katecodecompletion.cpp
katedocumenthelpers.cpp
katecodefoldinghelpers.cpp
kateviewinternal.cpp
katebookmarks.cpp
katefont.cpp
katelinerange.cpp
katesmartcursor.cpp
katerenderer.cpp
kateautoindent.cpp
katefiletype.cpp
kateschema.cpp
katetemplatehandler.cpp
katespell.cpp
kateprinter.cpp
kateindentscriptabstracts.cpp
kateluascript.cpp
katerange.cpp
katesmartrange.cpp
kateglobal.cpp
katecmd.cpp
katelayoutcache.cpp
katetextlayout.cpp
katesmartmanager.cpp
kateedit.cpp
katesmartcursornotifier.cpp
katerenderrange.cpp
katesmartregion.cpp
katedynamicanimation.cpp
kateextendedattribute.cpp
katecompletionwidget.cpp
katecompletionmodel.cpp
katecompletiondelegate.cpp
katecompletiontree.cpp
katejscript.cpp
)

kde4_automoc(${katepart_PART_SRCS})

set( katepart_DCOP_SKEL_SRCS
katedocument.h
)

kde4_add_dcop_skels(katepart_PART_SRCS ${katepart_DCOP_SKEL_SRCS})

kde4_add_plugin(katepart ${katepart_PART_SRCS})


target_link_libraries(katepart  ${QT_AND_KDECORE_LIBS} ktexteditor kdeprint kutils kjs knewstuff )

kde4_install_libtool_file(${KDE4_PLUGIN_INSTALL_DIR} katepart )
install_targets(${KDE4_PLUGIN_INSTALL_DIR} katepart )


########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = katepart.la
#
#katepart_la_SOURCES = katesearch.cpp katebuffer.cpp katecmds.cpp kateundo.cpp \
#	katecursor.cpp katedialogs.cpp katedocument.cpp katefactory.cpp katehighlight.cpp \
#	katesyntaxdocument.cpp katetextline.cpp kateview.cpp kateconfig.cpp kateviewhelpers.cpp \
#	katecodecompletion.cpp katedocumenthelpers.cpp katecodefoldinghelpers.cpp kateviewinternal.cpp \
#	katebookmarks.cpp katefont.cpp katelinerange.cpp katesmartcursor.cpp katerenderer.cpp \
#	kateautoindent.cpp katefiletype.cpp kateschema.cpp katedocument.skel \
#	katetemplatehandler.cpp katespell.cpp kateprinter.cpp \
#	kateindentscriptabstracts.cpp kateluascript.cpp katerange.cpp katesmartrange.cpp kateglobal.cpp \
#	katecmd.cpp katelayoutcache.cpp katetextlayout.cpp katesmartmanager.cpp \
#	kateedit.cpp katesmartcursornotifier.cpp katerenderrange.cpp katesmartregion.cpp \
#	katedynamicanimation.cpp kateextendedattribute.cpp
#
#katepart_la_LIBADD = $(top_builddir)/interfaces/ktexteditor/libktexteditor.la $(top_builddir)/kdeprint/libkdeprint.la $(top_builddir)/kutils/libkutils.la $(top_builddir)/kjs/libkjs.la $(top_builddir)/knewstuff/libknewstuff.la $(LUA_LIBS)
#
#katepart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
#
#INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/kdeprint \
#	-I$(top_srcdir)/interfaces -I$(top_srcdir)/interfaces/kregexpeditor -I$(top_srcdir)/kdefx \
#	-I$(top_srcdir)/kutils -I$(top_builddir)/kjs $(LUA_INCLUDES) $(all_includes)
#
#METASOURCES = AUTO
#
#LUT_FILES = katejscript.lut.h
#
#CREATE_HASH_TABLE = $(top_srcdir)/kjs/create_hash_table
#
#katejscript.lut.h : $(srcdir)/katejscript.cpp $(CREATE_HASH_TABLE)
#	$(PERL) $(CREATE_HASH_TABLE) $(srcdir)/katejscript.cpp > $@
#katejscript.lo: katejscript.lut.h
#
#CLEANFILES = $(LUT_FILES)
#noinst_HEADERS = katerenderrange.h katesmartregion.h katedynamicanimation.h \
#	kateextendedattribute.h
