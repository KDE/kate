lib_LTLIBRARIES = libkatepart.la

libkatepart_la_SOURCES = katebuffer.cpp katecmd.cpp katecmds.cpp katehledit_attrib.ui \
			 katedialogs.cpp katedocument.cpp katefactory.cpp katehighlight.cpp \
			 katesyntaxdocument.cpp katetextline.cpp kateview.cpp \
  			 kateviewdialog.cpp kateviewhighlightaction.cpp \
			 katehighlightdownload.cpp katefiledialog.cpp \
			 kateiconborder.cpp katecodecompletion_arghint.cpp \
			 katecodecompletion_iface_impl.cpp hlparamedit.cpp kateexportaction.cpp


libkatepart_la_LIBADD   = ../interfaces/libkatepartinterfaces.la $(top_builddir)/kspell/libkspell.la $(top_builddir)/kdeprint/libkdeprint.la
libkatepart_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(KDE_PLUGIN) -module -avoid-version

INCLUDES= $(all_includes) -I../interfaces -I$(top_builddir)/kspell -I$(top_builddir)/kdeprint -I$(top_srcdir)/interfaces

METASOURCES = AUTO
