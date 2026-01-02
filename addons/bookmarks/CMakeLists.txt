kate_add_plugin(bookmarksplugin)
target_compile_definitions(bookmarksplugin PRIVATE TRANSLATION_DOMAIN="katebookmarksplugin")
target_link_libraries(bookmarksplugin PRIVATE kateprivate KF6::I18n KF6::TextEditor)

target_sources(
  bookmarksplugin
  PRIVATE
    bookmarksplugin.cpp
    bookmarksmodel.cpp
)

if (ENABLE_PCH)
    target_precompile_headers(bookmarksplugin REUSE_FROM katepch)
endif()
