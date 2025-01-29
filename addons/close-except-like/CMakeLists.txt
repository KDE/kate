kate_add_plugin(katecloseexceptplugin)
target_compile_definitions(katecloseexceptplugin PRIVATE TRANSLATION_DOMAIN="katecloseexceptplugin")

target_link_libraries(
  katecloseexceptplugin
  PRIVATE
    KF6::I18n
    KF6::TextEditor
)

ki18n_wrap_ui(katecloseexceptplugin close_confirm_dialog.ui)
target_sources(
  katecloseexceptplugin
  PRIVATE
    close_confirm_dialog.cpp
    close_except_plugin.cpp
    plugin.qrc
)

if (ENABLE_PCH)
    target_precompile_headers(katecloseexceptplugin REUSE_FROM katepch)
endif()
