kate_add_plugin(bookmarksplugin)
target_compile_definitions(bookmarksplugin PRIVATE TRANSLATION_DOMAIN="bookmarksplugin")
target_link_libraries(bookmarksplugin PRIVATE kateprivate KF6::I18n KF6::TextEditor)

target_sources(
  bookmarksplugin
  PRIVATE
    bookmarksplugin.cpp
    bookmarksmodel.cpp
)
