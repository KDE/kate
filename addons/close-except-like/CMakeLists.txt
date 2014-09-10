project(katecloseexceptplugin)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 3)
set(VERSION_PATCH 5)
set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(KATE_CLOSE_EXCEPT_PLUGIN_SOURCES
    close_confirm_dialog.cpp
    close_except_plugin.cpp
  )

set(KATE_CLOSE_EXCEPT_PLUGIN_UI
    close_confirm_dialog.ui
)
ki18n_wrap_ui(KATE_CLOSE_EXCEPT_PLUGIN_SOURCES ${KATE_CLOSE_EXCEPT_PLUGIN_UI})

add_library(katecloseexceptplugin MODULE ${KATE_CLOSE_EXCEPT_PLUGIN_SOURCES})

target_link_libraries(katecloseexceptplugin
   KF5::TextEditor
   KF5::Parts
   KF5::I18n
   KF5::IconThemes
   KF5::Notifications
  )

configure_file(config.h.in config.h)

install(TARGETS katecloseexceptplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

kcoreaddons_desktop_to_json(katecloseexceptplugin katecloseexceptplugin.desktop)
install(FILES ui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}//katecloseexceptplugin)
