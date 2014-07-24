########### install template files ###############

install(
    FILES
      html.katetemplate
      cpplgpl.cpp.katetemplate
      cpplgpl.hh.katetemplate
      cppgpl.cpp.katetemplate
      cppgpl.hh.katetemplate
      docbookchapter.xml.katetemplate
      language.xml.katetemplate
      FindXXX.cmake.katetemplate
    DESTINATION ${DATA_INSTALL_DIR}/kate/plugins/katefiletemplates/templates
  )
