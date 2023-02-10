kate_add_plugin(terminalplugin)

add_subdirectory(qtermwidget)
#
target_sources(
  terminalplugin
  PRIVATE
    terminalplugin.h
    terminalplugin.cpp
    KateTerminalWidget.cpp
)
#
target_link_libraries(terminalplugin PRIVATE
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::TextEditor
    qtermwidget5
)

target_compile_definitions(terminalplugin PRIVATE TRANSLATION_DOMAIN="terminalplugin")
