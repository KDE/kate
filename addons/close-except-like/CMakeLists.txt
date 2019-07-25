set(KATE_CLOSE_EXCEPT_PLUGIN_SOURCES
  close_confirm_dialog.cpp
  close_except_plugin.cpp
  plugin.qrc
)

set(KATE_CLOSE_EXCEPT_PLUGIN_UI close_confirm_dialog.ui)

ki18n_wrap_ui(KATE_CLOSE_EXCEPT_PLUGIN_SOURCES ${KATE_CLOSE_EXCEPT_PLUGIN_UI})

add_library(katecloseexceptplugin MODULE ${KATE_CLOSE_EXCEPT_PLUGIN_SOURCES})
target_compile_definitions(katecloseexceptplugin PRIVATE TRANSLATION_DOMAIN="katecloseexceptplugin")

target_link_libraries(
  katecloseexceptplugin 
  PRIVATE
    KF5::TextEditor
    KF5::Parts
    KF5::I18n
    KF5::IconThemes
)

kcoreaddons_desktop_to_json(katecloseexceptplugin katecloseexceptplugin.desktop)
install(TARGETS katecloseexceptplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
