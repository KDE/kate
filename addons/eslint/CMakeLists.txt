kate_add_plugin(eslintplugin)
target_compile_definitions(eslintplugin PRIVATE TRANSLATION_DOMAIN="eslintplugin")
target_link_libraries(eslintplugin PRIVATE KF6::TextEditor kateprivate)

target_sources(
  eslintplugin
  PRIVATE
    EslintPlugin.cpp
)

if (BUILD_PCH)
    target_precompile_headers(eslintplugin REUSE_FROM katepch)
endif()
