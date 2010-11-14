



########### install files ###############

install( FILES katekjsconsolewindow.js katekjsconsolewindow.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katekjsconsolewindow )
install( FILES katekjstest1.desktop katekjsconsolewindow.desktop katekjsselect.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES katekjstest1.js  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katekjstest1 )
install( FILES katekjsselect.js katekjsselect.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katekjsselect )




#original Makefile.am contents follow:

#
## Install the .rc file in the Part's directory (in this case, the part
## is KHTMLPart)
##pluginsdir=$(kde_datadir)/kate/plugins/katekjswrapper
##plugins_DATA=ui.rc
#
#plugins1dir=$(kde_datadir)/kate/plugins/katekjstest1
#plugins1_DATA=katekjstest1.js
#
#pluginsconsolewindowdir=$(kde_datadir)/kate/plugins/katekjsconsolewindow
#pluginsconsolewindow_DATA=katekjsconsolewindow.js katekjsconsolewindow.rc
#
#pluginsselectdir=$(kde_datadir)/kate/plugins/katekjsselect
#pluginsselect_DATA=katekjsselect.js katekjsselect.rc
#
##plugins2dir = $(kde_datadir)/kate/plugins/katekjstest2
##plugins2_DATA = katekjstest2.js
#
#kde_services_DATA = katekjstest1.desktop katekjsconsolewindow.desktop katekjsselect.desktop
##kde_services_DATA = katekjstest2.desktop
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kateinsertcommand.pot
