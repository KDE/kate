kate_add_plugin(kateshownonprintableplugin)
target_compile_definitions(kateshownonprintableplugin PRIVATE TRANSLATION_DOMAIN="kateshownonprintableplugin")
target_link_libraries(kateshownonprintableplugin PRIVATE KF6::I18n KF6::TextEditor)

target_sources(
  kateshownonprintableplugin
  PRIVATE
    kateshownonprintableplugin.cpp
)

qt_add_resources(kateshownonprintableplugin plugin.qrc
    PREFIX "/kxmlgui5/kateshownonprintable"
    FILES ui.rc
)

if (ENABLE_PCH)
    target_precompile_headers(kateshownonprintableplugin REUSE_FROM katepch)
endif()
