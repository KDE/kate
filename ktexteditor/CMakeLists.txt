kde4_header()

include_directories( ${CMAKE_SOURCE_DIR}/kio ${CMAKE_SOURCE_DIR}/interfaces ${CMAKE_SOURCE_DIR}/kabc ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(ktexteditor_LIB_SRCS
ktexteditor.cpp
range.cpp
cursor.cpp
templateinterface.cpp
editorchooser.cpp
attribute.cpp
cursorfeedback.cpp
rangefeedback.cpp
smartcursor.cpp
smartrange.cpp
)

kde4_automoc(${ktexteditor_LIB_SRCS})

set( ktexteditor_UI
editorchooser_ui.ui
)

kde4_add_ui_files(ktexteditor_LIB_SRCS ${ktexteditor_UI} )

kde4_add_library(ktexteditor SHARED ${ktexteditor_LIB_SRCS})

target_link_libraries(ktexteditor  ${QT_AND_KDECORE_LIBS} kdocument kparts )

set_target_properties(ktexteditor PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install_targets(/lib ktexteditor )


########### next target ###############

set(ktexteditorkabcbridge_PART_SRCS
ktexteditorkabcbridge.cpp
)

kde4_automoc(${ktexteditorkabcbridge_PART_SRCS})

kde4_add_plugin(ktexteditorkabcbridge ${ktexteditorkabcbridge_PART_SRCS})

target_link_libraries(ktexteditorkabcbridge  ${QT_AND_KDECORE_LIBS} kabc )

install_targets(/lib/kde4 ktexteditorkabcbridge )


########### install files ###############
install_files( /include/ktexteditor FILES factory.h editor.h document.h view.h plugin.h      cursor.h range.h configpage.h searchinterface.h highlightinginterface.h       codecompletioninterface.h markinterface.h sessionconfiginterface.h editorchooser.h              texthintinterface.h variableinterface.h templateinterface.h           modificationinterface.h      commandinterface.h smartinterface.h attribute.h cursorfeedback.h rangefeedback.h    smartcursor.h smartrange.h )

install_files( ${KDE4_SERVICESTYPES_DIR} FILES ktexteditor.desktop ktexteditorplugin.desktop )
install_files( ${KDE4_DATA_DIR}/kcm_componentchooser FILES kcm_ktexteditor.desktop )

kde4_create_doxygen_docs( REFERENCES kdecore kdeui kparts  )

kde4_footer()



#original Makefile.am contents follow:

##KDE_CXXFLAGS = -UQT3_SUPPORT
#
#INCLUDES = -I$(top_srcdir)/kio -I$(top_srcdir) -I$(top_srcdir)/interfaces -I$(top_builddir)/interfaces  -I$(top_srcdir)/kabc -I$(top_builddir)/kabc $(all_includes)
#
#lib_LTLIBRARIES = libktexteditor.la
#
#libktexteditor_la_SOURCES = ktexteditor.cpp range.cpp cursor.cpp \
#	templateinterface.cpp 	editorchooser.cpp editorchooser_ui.ui attribute.cpp cursorfeedback.cpp \
#	rangefeedback.cpp smartcursor.cpp smartrange.cpp
#libktexteditor_la_LDFLAGS = -no-undefined $(all_libraries) $(KDE_RPATH)
#libktexteditor_la_LIBADD = $(top_builddir)/interfaces/kdocument/libkdocument.la $(LIB_KPARTS)
#
#ktexteditorinclude_HEADERS = factory.h editor.h document.h view.h plugin.h \
#		cursor.h range.h configpage.h searchinterface.h highlightinginterface.h \
#		codecompletioninterface.h markinterface.h sessionconfiginterface.h editorchooser.h 			 \
#		texthintinterface.h variableinterface.h templateinterface.h 			 modificationinterface.h \
#		commandinterface.h smartinterface.h attribute.h cursorfeedback.h rangefeedback.h \
#	smartcursor.h smartrange.h
#
#ktexteditorincludedir = $(includedir)/ktexteditor
#
#kde_module_LTLIBRARIES = ktexteditorkabcbridge.la
#ktexteditorkabcbridge_la_SOURCES=ktexteditorkabcbridge.cpp
#ktexteditorkabcbridge_la_LIBADD = $(LIB_KDECORE) $(top_builddir)/kabc/libkabc.la
#
#ktexteditorkabcbridge_la_LDFLAGS = -module $(all_libraries) $(KDE_PLUGIN)
#
#METASOURCES = AUTO
#
#servicetypedir = $(kde_servicetypesdir)
#servicetype_DATA = ktexteditor.desktop ktexteditorplugin.desktop
#
#kcm_ktexteditor_DATA = kcm_ktexteditor.desktop
#kcm_ktexteditordir = $(kde_datadir)/kcm_componentchooser
#
#templateinterface.lo: $(top_builddir)/kabc/addressee.h
#ktexteditorkabcbridge.lo: $(top_builddir)/kabc/addressee.h
#
#DOXYGEN_REFERENCES = kdecore kdeui kparts
#include ../../admin/Doxyfile.am
#
#noinst_HEADERS = attribute_p.h
