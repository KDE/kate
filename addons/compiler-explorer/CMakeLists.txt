kate_add_plugin(compilerexplorer)
target_compile_definitions(compilerexplorer PRIVATE TRANSLATION_DOMAIN="compilerexplorer")
target_link_libraries(compilerexplorer PRIVATE KF5::I18n KF5::TextEditor)

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

target_include_directories(
    compilerexplorer
    PUBLIC
    ${CMAKE_SOURCE_DIR}/shared
)

