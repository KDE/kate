INCLUDES = -I$(top_srcdir)/interfaces/ktexteditor -I$(top_srcdir)/kio -I$(top_srcdir) $(all_includes)


METASOURCES = AUTO

kde_module_LTLIBRARIES = kcm_ktexteditorchooser.la

kcm_ktexteditorchooser_la_SOURCES = kcm_ktexteditorchooser.cpp
kcm_ktexteditorchooser_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
kcm_ktexteditorchooser_la_LIBADD = ../libktexteditor.la $(LIB_KIO)

kcm_ktexteditorchooser_DATA = kcm_ktexteditorchooser.desktop
kcm_ktexteditorchooserdir = $(kde_appsdir)/Settings/Personalization

