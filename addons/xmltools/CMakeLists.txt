kate_add_plugin(katexmltoolsplugin)
target_compile_definitions(katexmltoolsplugin PRIVATE TRANSLATION_DOMAIN="katexmltools")
target_link_libraries(katexmltoolsplugin PRIVATE KF6::I18n KF6::TextEditor KF6::KIOWidgets)

target_sources(
  katexmltoolsplugin 
  PRIVATE
    pseudo_dtd.cpp
    plugin_katexmltools.cpp
    plugin.qrc
)

if (ENABLE_PCH)
    target_precompile_headers(katexmltoolsplugin REUSE_FROM katepch)
endif()

install(
  FILES 
    html4-loose.dtd.xml 
    html4-strict.dtd.xml 
    kde-docbook.dtd.xml 
    simplify_dtd.xsl
    xhtml1-frameset.dtd.xml 
    xhtml1-strict.dtd.xml 
    xhtml1-transitional.dtd.xml 
    xslt-1.0.dtd.xml
    testcases.xml 
    language.dtd.xml 
    kpartgui.dtd.xml 
    kcfg.dtd.xml
  DESTINATION ${KDE_INSTALL_DATADIR}/katexmltools
)
