kate_add_plugin(katecloseexceptplugin)
target_compile_definitions(katecloseexceptplugin PRIVATE TRANSLATION_DOMAIN="katecloseexceptplugin")

target_link_libraries(
  katecloseexceptplugin
  PRIVATE
    KF5::I18n
    KF5::TextEditor
)

ki18n_wrap_ui(katecloseexceptplugin close_confirm_dialog.ui)
target_sources(
  katecloseexceptplugin
  PRIVATE
    close_confirm_dialog.cpp
    close_except_plugin.cpp
    plugin.qrc
)

