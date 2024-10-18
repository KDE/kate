kate_add_plugin(katexmlcheckplugin)
target_compile_definitions(katexmlcheckplugin PRIVATE TRANSLATION_DOMAIN="katexmlcheck")
target_link_libraries(katexmlcheckplugin PRIVATE kateprivate KF6::I18n KF6::TextEditor)

target_sources(
  katexmlcheckplugin
  PRIVATE
    plugin_katexmlcheck.cpp
    plugin.qrc
)

if (BUILD_PCH)
    target_precompile_headers(katexmlcheckplugin REUSE_FROM katepch)
endif()
