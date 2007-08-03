



########### install files ###############

install( FILES html.katetemplate      cpplgpl.cpp.katetemplate cpplgpl.hh.katetemplate      cppgpl.cpp.katetemplate cppgpl.hh.katetemplate      docbookchapter.xml.katetemplate language.xml.katetemplate  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katefiletemplates/templates )




#original Makefile.am contents follow:

#templatesdir=$(kde_datadir)/kate/plugins/katefiletemplates/templates
#templates_DATA=html.katetemplate \
#    cpplgpl.cpp.katetemplate cpplgpl.hh.katetemplate \
#    cppgpl.cpp.katetemplate cppgpl.hh.katetemplate \
#    docbookchapter.xml.katetemplate language.xml.katetemplate
