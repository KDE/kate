include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_definitions(-DTRANSLATION_DOMAIN=\"kterustcompletion\")

set(kterustcompletion_SRCS
  kterustcompletion.cpp
  kterustcompletionconfigpage.cpp
  kterustcompletionplugin.cpp
  kterustcompletionpluginview.cpp
)

# resource for ui file and stuff
qt5_add_resources(kterustcompletion_SRCS plugin.qrc)

add_library(kterustcompletionplugin MODULE ${kterustcompletion_SRCS})

kcoreaddons_desktop_to_json(kterustcompletionplugin kterustcompletionplugin.desktop)

target_link_libraries(kterustcompletionplugin
  KF5::TextEditor
  KF5::XmlGui
)

install(TARGETS kterustcompletionplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
