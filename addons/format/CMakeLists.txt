kate_add_plugin(formatplugin)
target_compile_definitions(formatplugin PRIVATE TRANSLATION_DOMAIN="formatplugin")
target_link_libraries(formatplugin PRIVATE KF6::TextEditor kateprivate)

target_sources(
  formatplugin
  PRIVATE
    FormatPlugin.cpp
    Formatters.cpp
    FormatConfig.cpp
    plugin.qrc
)

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()

if (ENABLE_PCH)
    target_precompile_headers(formatplugin REUSE_FROM katepch)
endif()
