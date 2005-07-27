INCLUDES= -I$(srcdir)/../interfaces -I$(srcdir)/../part -I$(top_srcdir) -I$(top_srcdir)/kdeprint -I$(top_srcdir)/interfaces  -I$(top_srcdir)/interfaces/kregexpeditor -I$(top_srcdir)/kdefx -I$(top_srcdir)/kutils -I$(top_builddir)/kjs $(all_includes)

METASOURCES = AUTO

check_PROGRAMS = katetest

katetest_SOURCES = katetest.cpp
katetest_LDADD = ../part/libkatepart.la
katetest_LDFLAGS = $(KDE_RPATH)
