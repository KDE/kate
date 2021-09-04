include(ECMQtDeclareLoggingCategory)
add_subdirectory(hat-trie)

add_library(latexcompletionplugin MODULE)
target_compile_definitions(latexcompletionplugin PRIVATE TRANSLATION_DOMAIN="latexcompletionplugin")
ecm_qt_declare_logging_category(
  latexcompletionplugin
  HEADER logging.h
  IDENTIFIER LATEXCOMPLETION
  CATEGORY_NAME "katelatexcompletionplugin"
)

kde_target_enable_exceptions(latexcompletionplugin PRIVATE)
target_link_libraries(latexcompletionplugin PRIVATE KF5::I18n KF5::TextEditor tsl::hat_trie)

target_sources(
  latexcompletionplugin
  PRIVATE
    latexcompletionplugin.cpp
    completionmodel.cpp
)

# ensure we are able to load plugins pre-install, too, directories must match!
set_target_properties(latexcompletionplugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/ktexteditor")
install(TARGETS latexcompletionplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/ktexteditor)
