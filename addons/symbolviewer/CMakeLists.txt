kate_add_plugin(katesymbolviewerplugin)
target_compile_definitions(katesymbolviewerplugin PRIVATE TRANSLATION_DOMAIN="katesymbolviewer")
target_link_libraries(
    katesymbolviewerplugin
    PRIVATE
        kateprivate
        KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::TextEditor
)

target_sources(
  katesymbolviewerplugin
  PRIVATE
    cpp_parser.cpp
    tcl_parser.cpp
    fortran_parser.cpp
    perl_parser.cpp
    php_parser.cpp
    xslt_parser.cpp
    xml_parser.cpp
    ruby_parser.cpp
    python_parser.cpp
    bash_parser.cpp
    ecma_parser.cpp
    plugin_katesymbolviewer.cpp
    plugin.qrc
)

