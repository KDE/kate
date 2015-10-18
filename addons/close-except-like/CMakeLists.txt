project(katecloseexceptplugin)
add_definitions(-DTRANSLATION_DOMAIN=\"katecloseexceptplugin\")
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

# resource for ui file and stuff
qt5_add_resources(KATE_CLOSE_EXCEPT_PLUGIN_SOURCES plugin.qrc)

add_library(katecloseexceptplugin MODULE ${KATE_CLOSE_EXCEPT_PLUGIN_SOURCES})

# we compile in the .desktop file
kcoreaddons_desktop_to_json(katecloseexceptplugin katecloseexceptplugin.desktop)

target_link_libraries(katecloseexceptplugin
   KF5::TextEditor
   KF5::Parts
   KF5::I18n
   KF5::IconThemes
  )

configure_file(config.h.in config.h)

install(TARGETS katecloseexceptplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
