kate_add_plugin(keyboardmacrosplugin)
target_compile_definitions(keyboardmacrosplugin PRIVATE TRANSLATION_DOMAIN="katekeyboardmacros")
target_link_libraries(keyboardmacrosplugin PRIVATE kateprivate KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::TextEditor)

target_sources(
  keyboardmacrosplugin
  PRIVATE
    keyboardmacrosplugin.cpp
    keyboardmacrospluginview.cpp
    keyboardmacrosplugincommands.cpp
    plugin.qrc
)
