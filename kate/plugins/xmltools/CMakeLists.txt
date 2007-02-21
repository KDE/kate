



########### next target ###############

set(katexmltoolsplugin_PART_SRCS pseudo_dtd.cpp plugin_katexmltools.cpp )

kde4_automoc(${katexmltoolsplugin_PART_SRCS})

kde4_add_plugin(katexmltoolsplugin ${katexmltoolsplugin_PART_SRCS})



target_link_libraries(katexmltoolsplugin  ${KDE4_KDECORE_LIBS} kateinterfaces )

install(TARGETS katexmltoolsplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katexmltools )
install( FILES html4-loose.dtd.xml  html4-strict.dtd.xml kde-docbook.dtd.xml  	simplify_dtd.xsl xhtml1-frameset.dtd.xml xhtml1-strict.dtd.xml  	xhtml1-transitional.dtd.xml xslt-1.0.dtd.xml  	testcases.xml language.dtd.xml kpartgui.dtd.xml kcfg.dtd.xml  DESTINATION  ${DATA_INSTALL_DIR}/katexmltools )
install( FILES katexmltools.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )




#original Makefile.am contents follow:

#INCLUDES  =  $(all_includes)
#METASOURCES = AUTO
#
## Install this plugin in the KDE modules directory
#kde_module_LTLIBRARIES = katexmltoolsplugin.la
#
## This is all standard.  Remove the LIB_KHTML reference if you are not
## using the KHTML Part
#katexmltoolsplugin_la_SOURCES = pseudo_dtd.cpp plugin_katexmltools.cpp
#katexmltoolsplugin_la_LIBADD = -lkateinterfaces
#katexmltoolsplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
## Install the .rc file in the Part's directory (in this case, the part
## is KHTMLPart)
#pluginsdir = $(kde_datadir)/kate/plugins/katexmltools
#plugins_DATA = ui.rc
#
#xmldir = $(kde_datadir)/katexmltools
#xml_DATA = html4-loose.dtd.xml  html4-strict.dtd.xml kde-docbook.dtd.xml \
#	simplify_dtd.xsl xhtml1-frameset.dtd.xml xhtml1-strict.dtd.xml \
#	xhtml1-transitional.dtd.xml xslt-1.0.dtd.xml \
#	testcases.xml language.dtd.xml kpartgui.dtd.xml kcfg.dtd.xml
#
##deskdir=$(kde_datadir)/kate/plugins
##desk_DATA=katexmltools.desktop
#
#kde_services_DATA = katexmltools.desktop
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/katexmltools.pot
