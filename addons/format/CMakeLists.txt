kate_add_plugin(formatplugin)
target_compile_definitions(formatplugin PRIVATE TRANSLATION_DOMAIN="formatplugin")
target_link_libraries(formatplugin PRIVATE KF${KF_MAJOR_VERSION}::TextEditor kateprivate)

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
