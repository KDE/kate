kate_add_plugin(formatplugin)
target_compile_definitions(formatplugin PRIVATE TRANSLATION_DOMAIN="formatplugin")
target_link_libraries(formatplugin PRIVATE KF5::TextEditor kateprivate)

target_sources(
  formatplugin
  PRIVATE
    FormatPlugin.cpp
    Formatters.cpp
    FormatConfig.cpp
    plugin.qrc
)

