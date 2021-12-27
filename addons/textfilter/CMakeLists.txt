kate_add_plugin(textfilterplugin)
target_compile_definitions(textfilterplugin PRIVATE TRANSLATION_DOMAIN="katetextfilter")
target_link_libraries(textfilterplugin PRIVATE KF5::I18n KF5::TextEditor)

ki18n_wrap_ui(UI_SOURCES textfilterwidget.ui)
target_sources(textfilterplugin PRIVATE ${UI_SOURCES})

target_sources(
  textfilterplugin
  PRIVATE
    plugin_katetextfilter.cpp
    plugin.qrc
)

