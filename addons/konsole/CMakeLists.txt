kate_add_plugin(katekonsoleplugin)
target_compile_definitions(katekonsoleplugin PRIVATE TRANSLATION_DOMAIN="katekonsoleplugin")
target_link_libraries(katekonsoleplugin PRIVATE KF6::I18n KF6::TextEditor)

target_sources(
  katekonsoleplugin 
  PRIVATE
    kateconsole.cpp 
    plugin.qrc
)

if (ENABLE_PCH)
    target_precompile_headers(katekonsoleplugin REUSE_FROM katepch)
endif()
