INCLUDES = -I$(srcdir)/../interfaces \
	-I$(srcdir)/../part -I$(top_srcdir) -I$(top_srcdir)/kdeprint -I$(top_srcdir)/interfaces \
	-I$(top_srcdir)/interfaces/kregexpeditor -I$(top_srcdir)/kdefx -I$(top_srcdir)/kutils -I$(top_builddir)/kjs \
	$(all_includes)

METASOURCES = AUTO

check_PROGRAMS = katetest kateregression

kateregression_SOURCES = kateregression.cpp rangeexpectation.cpp \
	cursorexpectation.cpp
kateregression_LDADD = $(top_builddir)/interfaces/ktexteditor/libktexteditor.la -lQtTest_debug
kateregression_LDFLAGS = $(KDE_RPATH) $(all_libraries)

katetest_SOURCES = katetest.cpp arbitraryhighlighttest.cpp \
	codecompletiontest.cpp
katetest_LDADD = $(top_builddir)/interfaces/ktexteditor/libktexteditor.la
katetest_LDFLAGS = $(KDE_RPATH) $(all_libraries)

TESTS = kateregression

noinst_HEADERS = rangeexpectation.h cursorexpectation.h \
		arbitraryhighlighttest.h codecompletiontest.h
