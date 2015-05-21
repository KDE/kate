include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_definitions(-DTRANSLATION_DOMAIN=\"kterustcompletion\")

set(kterustcompletion_SRCS
  kterustcompletion.cpp
  kterustcompletionconfigpage.cpp
  kterustcompletionplugin.cpp
  kterustcompletionpluginview.cpp
)

add_library(kterustcompletionplugin MODULE ${kterustcompletion_SRCS})

target_link_libraries(kterustcompletionplugin
  KF5::TextEditor
  KF5::XmlGui
)

install(TARGETS kterustcompletionplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

install(FILES ui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/kterustcompletion)

kcoreaddons_desktop_to_json(kterustcompletionplugin kterustcompletionplugin.desktop)
