kate_add_plugin(cmaketoolsplugin)
target_compile_definitions(cmaketoolsplugin PRIVATE TRANSLATION_DOMAIN="cmaketoolsplugin")
target_link_libraries(cmaketoolsplugin PRIVATE kateprivate KF6::I18n KF6::TextEditor)

target_sources(
  cmaketoolsplugin
  PRIVATE
    cmaketoolsplugin.cpp
    cmakecompletion.cpp
)

if (BUILD_PCH)
    target_precompile_headers(cmaketoolsplugin REUSE_FROM katepch)
endif()
