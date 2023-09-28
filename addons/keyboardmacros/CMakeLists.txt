kate_add_plugin(keyboardmacrosplugin)
target_compile_definitions(keyboardmacrosplugin PRIVATE TRANSLATION_DOMAIN="katekeyboardmacros")
target_link_libraries(keyboardmacrosplugin PRIVATE kateprivate KF6::I18n KF6::TextEditor)

target_sources(
  keyboardmacrosplugin
  PRIVATE
    keyboardmacrosplugin.cpp
    keyboardmacrospluginview.cpp
    keyboardmacrosplugincommands.cpp
    plugin.qrc
)

if (BUILD_PCH)
    target_precompile_headers(keyboardmacrosplugin REUSE_FROM katepch)
endif()
