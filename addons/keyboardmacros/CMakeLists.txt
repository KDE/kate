kate_add_plugin(keyboardmacrosplugin)
target_compile_definitions(keyboardmacrosplugin PRIVATE TRANSLATION_DOMAIN="katekeyboardmacros")
target_link_libraries(keyboardmacrosplugin PRIVATE kateprivate KF5::I18n KF5::TextEditor)

target_sources(
  keyboardmacrosplugin
  PRIVATE
    keyboardmacrosplugin.cpp
    keyboardmacrospluginview.cpp
    keyboardmacrosplugincommands.cpp
    plugin.qrc
)
