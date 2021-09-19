add_library(compilerexplorer MODULE "")
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

# ensure we are able to load plugins pre-install, too, directories must match!
set_target_properties(compilerexplorer PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/ktexteditor")
install(TARGETS compilerexplorer DESTINATION ${KDE_INSTALL_PLUGINDIR}/ktexteditor)
