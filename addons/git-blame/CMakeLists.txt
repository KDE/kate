kate_add_plugin(kategitblameplugin)
target_compile_definitions(kategitblameplugin PRIVATE TRANSLATION_DOMAIN="kategitblameplugin")
target_link_libraries(kategitblameplugin PRIVATE kateprivate KF6::I18n KF6::TextEditor)

target_sources(
  kategitblameplugin
  PRIVATE
    kategitblameplugin.cpp
    gitblametooltip.cpp
    gitblameparser.cpp
    plugin.qrc
)

if (ENABLE_PCH)
    target_precompile_headers(kategitblameplugin REUSE_FROM katepch)
endif()
