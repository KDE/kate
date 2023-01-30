kate_add_plugin(kategitblameplugin)
target_compile_definitions(kategitblameplugin PRIVATE TRANSLATION_DOMAIN="kategitblameplugin")
target_link_libraries(kategitblameplugin PRIVATE kateprivate KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::TextEditor)

target_sources(
  kategitblameplugin
  PRIVATE
    kategitblameplugin.cpp
    gitblametooltip.cpp
    plugin.qrc
)
