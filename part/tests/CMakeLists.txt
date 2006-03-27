kde4_header()

if(KDE4_BUILD_TESTS)

# parent included kio already
include_directories( ${CMAKE_SOURCE_DIR}/kdeprint ${CMAKE_SOURCE_DIR}/interfaces ${CMAKE_SOURCE_DIR}/interfaces/kregexpeditor ${CMAKE_SOURCE_DIR}/kutils )


########### next target ###############

set(katetest_SRCS
katetest.cpp
arbitraryhighlighttest.cpp
codecompletiontest.cpp
)

kde4_automoc(${katetest_SRCS})

kde4_add_executable(katetest ${katetest_SRCS})

target_link_libraries(katetest  ${KDE4_KDECORE_LIBS} ktexteditor )


########### next target ###############

set(kateregression_SRCS
kateregression.cpp
rangeexpectation.cpp
cursorexpectation.cpp
)

kde4_automoc(${kateregression_SRCS})

kde4_add_executable(kateregression ${kateregression_SRCS})

target_link_libraries(kateregression  ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} ktexteditor )

endif(KDE4_BUILD_TESTS)

########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = -I$(srcdir)/../interfaces \
#	-I$(srcdir)/../part -I$(top_srcdir) -I$(top_srcdir)/kdeprint -I$(top_srcdir)/interfaces \
#	-I$(top_srcdir)/interfaces/kregexpeditor -I$(top_srcdir)/kdefx -I$(top_srcdir)/kutils -I$(top_builddir)/kjs \
#	$(all_includes)
#
#METASOURCES = AUTO
#
#check_PROGRAMS = katetest kateregression
#
#kateregression_SOURCES = kateregression.cpp rangeexpectation.cpp \
#	cursorexpectation.cpp
#kateregression_LDADD = $(top_builddir)/interfaces/ktexteditor/libktexteditor.la -lQtTest_debug
#kateregression_LDFLAGS = $(KDE_RPATH) $(all_libraries)
#
#katetest_SOURCES = katetest.cpp arbitraryhighlighttest.cpp
#katetest_LDADD = $(top_builddir)/interfaces/ktexteditor/libktexteditor.la
#katetest_LDFLAGS = $(KDE_RPATH) $(all_libraries)
#
#TESTS = kateregression
#
#noinst_HEADERS = rangeexpectation.h cursorexpectation.h \
#	arbitraryhighlighttest.h
