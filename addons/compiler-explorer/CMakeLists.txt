kate_add_plugin(compilerexplorer)
target_compile_definitions(compilerexplorer PRIVATE TRANSLATION_DOMAIN="compilerexplorer")
target_link_libraries(compilerexplorer PRIVATE kateprivate KF6::I18n KF6::TextEditor)

target_sources(
  compilerexplorer
  PRIVATE
    ce_plugin.cpp
    ce_service.cpp
    ce_widget.cpp
    AsmView.cpp
    AsmViewModel.cpp
    compiledbreader.cpp
)

if (ENABLE_PCH)
    target_precompile_headers(compilerexplorer REUSE_FROM katepch)
endif()
