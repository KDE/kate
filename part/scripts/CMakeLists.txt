


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/katepart/scripts/indent FILES script-indent-c-test.js script-indent-c1-test.lua )
install_files( ${DATA_INSTALL_DIR}/katepart/scripts FILES jstest.desktop jstest.js script-indent-c-newline.js script-indent-c-char.js sort.js )




#original Makefile.am contents follow:

#scriptskatedir = $(kde_datadir)/katepart/scripts
#scriptskate_DATA = jstest.desktop jstest.js script-indent-c-newline.js script-indent-c-char.js sort.js
#
#indentscriptskatedir = $(kde_datadir)/katepart/scripts/indent
#indentscriptskate_DATA = script-indent-c-test.js script-indent-c1-test.lua
