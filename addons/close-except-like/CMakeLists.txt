kate_add_plugin(katecloseexceptplugin)
target_compile_definitions(katecloseexceptplugin PRIVATE TRANSLATION_DOMAIN="katecloseexceptplugin")

target_link_libraries(
  katecloseexceptplugin
  PRIVATE
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::TextEditor
)

ki18n_wrap_ui(katecloseexceptplugin close_confirm_dialog.ui)
target_sources(
  katecloseexceptplugin
  PRIVATE
    close_confirm_dialog.cpp
    close_except_plugin.cpp
    plugin.qrc
)

