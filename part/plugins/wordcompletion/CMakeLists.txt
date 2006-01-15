kde4_header()

include_directories( ${CMAKE_SOURCE_DIR}/interfaces ${KDE4_INCLUDE_DIR} ${QT_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(ktexteditor_docwordcompletion_PART_SRCS
docwordcompletion.cpp
)

kde4_automoc(${ktexteditor_docwordcompletion_PART_SRCS})

kde4_add_plugin(ktexteditor_docwordcompletion ${ktexteditor_docwordcompletion_PART_SRCS})

target_link_libraries(ktexteditor_docwordcompletion  ${QT_AND_KDECORE_LIBS} ktexteditor )

install_targets(/lib/kde4 ktexteditor_docwordcompletion )


########### install files ###############

install_files( ${KDE4_DATA_DIR}/ktexteditor_docwordcompletion FILES docwordcompletionui.rc )
install_files( ${KDE4_SERVICES_DIR} FILES ktexteditor_docwordcompletion.desktop )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES    = -I$(top_srcdir)/interfaces $(all_includes)
#METASOURCES = AUTO
#
## Install this plugin in the KDE modules directory
#kde_module_LTLIBRARIES = ktexteditor_docwordcompletion.la
#
#ktexteditor_docwordcompletion_la_SOURCES = docwordcompletion.cpp
#ktexteditor_docwordcompletion_la_LIBADD = $(top_builddir)/interfaces/ktexteditor/libktexteditor.la
#ktexteditor_docwordcompletion_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
#docwordcompletiondatadir = $(kde_datadir)/ktexteditor_docwordcompletion
#docwordcompletiondata_DATA = docwordcompletionui.rc
#
#kde_services_DATA = ktexteditor_docwordcompletion.desktop
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp *.h -o $(podir)/ktexteditor_docwordcompletion.pot
