INCLUDES = -I$(top_srcdir)/kunittest -I$(srcdir)/../interfaces \
	-I$(srcdir)/../part -I$(top_srcdir) -I$(top_srcdir)/kdeprint -I$(top_srcdir)/interfaces \
	-I$(top_srcdir)/interfaces/kregexpeditor -I$(top_srcdir)/kdefx -I$(top_srcdir)/kutils -I$(top_builddir)/kjs \
	$(all_includes)

METASOURCES = AUTO

check_LTLIBRARIES = kunittest_kate.la
kunittest_kate_la_SOURCES = kateregression.cpp
kunittest_kate_la_LIBADD = $(top_builddir)/kate/part/katepart.la $(LIB_KUNITTEST)
kunittest_kate_la_LDFLAGS = -module $(KDE_CHECK_PLUGIN) $(all_libraries)

#check_PROGRAMS = katetest

#katetest_SOURCES = katetest.cpp
#katetest_LDADD = $(top_builddir)/kate/part/katepart.la
#katetest_LDFLAGS = $(KDE_RPATH)
