kate_add_plugin(katepythonplugin)
target_compile_definitions(katepythonplugin PRIVATE TRANSLATION_DOMAIN="katepythonplugin")

target_link_libraries(
  katepythonplugin
  PRIVATE
    KF5::I18n
    KF5::TextEditor
)

target_sources(
  katepythonplugin
  PRIVATE
    katepythonplugin.cpp
    plugin.qrc
)
