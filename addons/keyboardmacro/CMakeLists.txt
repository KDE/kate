kate_add_plugin(keyboardmacroplugin)
target_compile_definitions(keyboardmacroplugin PRIVATE TRANSLATION_DOMAIN="katekeyboardmacro")
target_link_libraries(keyboardmacroplugin PRIVATE KF5::I18n KF5::TextEditor)

ki18n_wrap_ui(keyboardmacroplugin keyboardmacrowidget.ui)

target_sources(
  keyboardmacroplugin
  PRIVATE
    plugin_katekeyboardmacro.cpp
    plugin.qrc
)

