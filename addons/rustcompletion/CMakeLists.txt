add_library(kterustcompletionplugin MODULE ${kterustcompletion_SRCS})
target_compile_definitions(kterustcompletionplugin PRIVATE TRANSLATION_DOMAIN="kterustcompletion")
target_link_libraries(kterustcompletionplugin PRIVATE KF5::TextEditor)

target_sources(
  kterustcompletionplugin 
  PRIVATE
    kterustcompletion.cpp
    kterustcompletionconfigpage.cpp
    kterustcompletionplugin.cpp
    kterustcompletionpluginview.cpp
    plugin.qrc
)

kcoreaddons_desktop_to_json(kterustcompletionplugin kterustcompletionplugin.desktop)
install(TARGETS kterustcompletionplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
