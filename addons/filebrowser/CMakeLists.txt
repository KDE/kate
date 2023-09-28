kate_add_plugin(katefilebrowserplugin)
target_compile_definitions(katefilebrowserplugin PRIVATE TRANSLATION_DOMAIN="katefilebrowserplugin")

target_link_libraries(
  katefilebrowserplugin 
  PRIVATE
    kateprivate
    KF6::I18n
    KF6::TextEditor 
    KF6::KIOFileWidgets
    KF6::Bookmarks
)

target_sources(
  katefilebrowserplugin
  PRIVATE
    katefilebrowserplugin.cpp
    katefilebrowserconfig.cpp
    katefilebrowser.cpp
    katebookmarkhandler.cpp
    katefilebrowseropenwithmenu.cpp
)

if (BUILD_PCH)
    target_precompile_headers(katefilebrowserplugin REUSE_FROM katepch)
endif()
