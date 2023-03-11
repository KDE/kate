kate_add_plugin(ktexteditorpreviewplugin)
target_compile_definitions(ktexteditorpreviewplugin PRIVATE TRANSLATION_DOMAIN="ktexteditorpreviewplugin")
target_link_libraries(ktexteditorpreviewplugin PRIVATE KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::TextEditor)

include(ECMQtDeclareLoggingCategory)
ecm_qt_declare_logging_category(
  DEBUG_SOURCES
  HEADER ktexteditorpreview_debug.h
  IDENTIFIER KTEPREVIEW
  CATEGORY_NAME "ktexteditorpreviewplugin"
)
target_sources(ktexteditorpreviewplugin PRIVATE ${DEBUG_SOURCES})

target_sources(
  ktexteditorpreviewplugin
  PRIVATE
    ktexteditorpreviewplugin.cpp
    ktexteditorpreviewview.cpp
    previewwidget.cpp
    kpartview.cpp
)

if (BUILD_PCH)
    target_precompile_headers(ktexteditorpreviewplugin REUSE_FROM katepch)
endif()
