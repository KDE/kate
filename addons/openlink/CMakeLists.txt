kate_add_plugin(openlinkplugin)
target_compile_definitions(openlinkplugin
                           PRIVATE TRANSLATION_DOMAIN="openlinkplugin")
target_link_libraries(openlinkplugin PRIVATE KF6::TextEditor KF6::I18n)

target_sources(openlinkplugin PRIVATE OpenLinkPlugin.cpp)

if(BUILD_PCH)
  target_precompile_headers(openlinkplugin REUSE_FROM katepch)
endif()
