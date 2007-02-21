



########### next target ###############

set(katehelloworldplugin_PART_SRCS plugin_katehelloworld.cpp )

kde4_automoc(${katehelloworldplugin_PART_SRCS})

kde4_add_plugin(katehelloworldplugin ${katehelloworldplugin_PART_SRCS})



target_link_libraries(katehelloworldplugin  ${KDE4_KDEUI_LIBS} kateinterfaces )

install(TARGETS katehelloworldplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katehelloworld )
install( FILES katehelloworld.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )




#original Makefile.am contents follow:

#INCLUDES    =  $(all_includes)
#METASOURCES = AUTO
#
## Install this plugin in the KDE modules directory
#kde_module_LTLIBRARIES = katehelloworldplugin.la
#
#katehelloworldplugin_la_SOURCES = plugin_katehelloworld.cpp
#katehelloworldplugin_la_LIBADD = -lkateinterfaces
#katehelloworldplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
#pluginsdir = $(kde_datadir)/kate/plugins/katehelloworld
#plugins_DATA = ui.rc
#
#kde_services_DATA = katehelloworld.desktop
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/katehelloworld.pot
#
