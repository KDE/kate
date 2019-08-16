find_package(KF5IconThemes QUIET)
set_package_properties(KF5IconThemes PROPERTIES PURPOSE "Required to build the close-except-like addon")

if(NOT KF5IconThemes_FOUND)
  return()
endif()

add_library(katecloseexceptplugin MODULE "")
target_compile_definitions(katecloseexceptplugin PRIVATE TRANSLATION_DOMAIN="katecloseexceptplugin")

target_link_libraries(
  katecloseexceptplugin 
  PRIVATE
    KF5::TextEditor
    KF5::IconThemes
)

ki18n_wrap_ui(UI_SOURCES close_confirm_dialog.ui)
target_sources(katecloseexceptplugin PRIVATE ${UI_SOURCES})

target_sources(
  katecloseexceptplugin
  PRIVATE
    close_confirm_dialog.cpp
    close_except_plugin.cpp
    plugin.qrc
)

kcoreaddons_desktop_to_json(katecloseexceptplugin katecloseexceptplugin.desktop)
install(TARGETS katecloseexceptplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
