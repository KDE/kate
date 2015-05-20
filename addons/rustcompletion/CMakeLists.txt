include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(kterustcompletion_SRCS
  kterustcompletion.cpp
  kterustcompletionconfigpage.cpp
  kterustcompletionplugin.cpp
  kterustcompletionpluginview.cpp
)

add_library(kterustcompletionplugin MODULE ${kterustcompletion_SRCS})

target_link_libraries(kterustcompletionplugin
  KF5::KIOFileWidgets
  KF5::TextEditor
)

install(TARGETS kterustcompletionplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

kcoreaddons_desktop_to_json(kterustcompletionplugin kterustcompletionplugin.desktop)
