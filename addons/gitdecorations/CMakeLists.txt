kate_add_plugin(gitdecorationsplugin)
target_compile_definitions(gitdecorationsplugin PRIVATE TRANSLATION_DOMAIN="kategitdecorationsplugin")
target_link_libraries(gitdecorationsplugin PRIVATE kateprivate KF6::I18n KF6::TextEditor)

target_sources(
  gitdecorationsplugin
  PRIVATE
    gitdecorationsplugin.cpp
    gitannotationmodel.cpp
    gitannotationdelegate.cpp
)

if (ENABLE_PCH)
    target_precompile_headers(gitdecorationsplugin REUSE_FROM katepch)
endif()
