kate_add_plugin(katesymbolviewerplugin)
target_compile_definitions(katesymbolviewerplugin PRIVATE TRANSLATION_DOMAIN="katesymbolviewer")
target_link_libraries(
    katesymbolviewerplugin
    PRIVATE
        kateprivate
        KF6::I18n KF6::TextEditor
)

target_sources(
  katesymbolviewerplugin
  PRIVATE
    plugin_katesymbolviewer.cpp
    plugin.qrc
)

if (BUILD_PCH)
    target_precompile_headers(katesymbolviewerplugin REUSE_FROM katepch)
endif()
