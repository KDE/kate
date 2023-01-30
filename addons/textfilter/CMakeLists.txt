kate_add_plugin(textfilterplugin)
target_compile_definitions(textfilterplugin PRIVATE TRANSLATION_DOMAIN="katetextfilter")
target_link_libraries(textfilterplugin PRIVATE KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::TextEditor)

ki18n_wrap_ui(textfilterplugin textfilterwidget.ui)

target_sources(
  textfilterplugin
  PRIVATE
    plugin_katetextfilter.cpp
    plugin.qrc
)

