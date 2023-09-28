kate_add_plugin(textfilterplugin)
target_compile_definitions(textfilterplugin PRIVATE TRANSLATION_DOMAIN="katetextfilter")
target_link_libraries(textfilterplugin PRIVATE KF6::I18n KF6::TextEditor)

ki18n_wrap_ui(textfilterplugin textfilterwidget.ui)

target_sources(
  textfilterplugin
  PRIVATE
    plugin_katetextfilter.cpp
    plugin.qrc
)

if (BUILD_PCH)
    target_precompile_headers(textfilterplugin REUSE_FROM katepch)
endif()
