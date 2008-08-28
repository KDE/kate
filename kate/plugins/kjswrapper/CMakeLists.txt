
add_subdirectory( samples ) 




########### next target ###############

set(katekjswrapperplugin_PART_SRCS plugin_katekjswrapper.cpp bindings.cpp )


kde4_add_plugin(katekjswrapperplugin ${katekjswrapperplugin_PART_SRCS})



target_link_libraries(katekjswrapperplugin ${KDE4_KDECORE_LIBS} kateinterfaces kjsembed ${KDE4_KJS_LIBS})

install(TARGETS katekjswrapperplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############





#original Makefile.am contents follow:

#SUBDIRS= . samples
#INCLUDES    =  $(all_includes)
#METASOURCES = AUTO
#
## Install this plugin in the KDE modules directory
#kde_module_LTLIBRARIES = katekjswrapperplugin.la
#
## This is all standard.  Remove the LIB_KHTML reference if you are not
## using the KHTML Part
#katekjswrapperplugin_la_SOURCES = plugin_katekjswrapper.cpp bindings.cpp
#katekjswrapperplugin_la_LIBADD = -lkateinterfaces -lkjsembed -lkjs
#katekjswrapperplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/katekjswrapper.pot
#
