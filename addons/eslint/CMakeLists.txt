kate_add_plugin(eslintplugin)
target_compile_definitions(eslintplugin PRIVATE TRANSLATION_DOMAIN="eslintplugin")
target_link_libraries(eslintplugin PRIVATE KF5::TextEditor kateprivate)

target_sources(
  eslintplugin
  PRIVATE
    EslintPlugin.cpp
)

