



########### next target ###############

set(katetextfilterplugin_PART_SRCS plugin_katetextfilter.cpp )


kde4_add_plugin(katetextfilterplugin ${katetextfilterplugin_PART_SRCS})



target_link_libraries(katetextfilterplugin  ${KDE4_KDEUI_LIBS} ${KDE4_KTEXTEDITOR_LIBS} kateinterfaces )

install(TARGETS katetextfilterplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katetextfilter )
install( FILES katetextfilter.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )




#original Makefile.am contents follow:

#INCLUDES    = $(all_includes)
#METASOURCES = AUTO
#
## Install this plugin in the KDE modules directory
#kde_module_LTLIBRARIES = katetextfilterplugin.la
#
## This is all standard.  Remove the LIB_KHTML reference if you are not
## using the KHTML Part
#katetextfilterplugin_la_SOURCES = plugin_katetextfilter.cpp
#katetextfilterplugin_la_LIBADD = -lkateinterfaces
#katetextfilterplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
## Install the .rc file in the Part's directory (in this case, the part
## is KHTMLPart)
#pluginsdir = $(kde_datadir)/kate/plugins/katetextfilter
#plugins_DATA = ui.rc
#
#kde_services_DATA = katetextfilter.desktop
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/katetextfilter.pot
