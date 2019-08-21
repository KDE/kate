add_library(textfilterplugin MODULE "")
target_compile_definitions(textfilterplugin PRIVATE TRANSLATION_DOMAIN="katetextfilter")
target_link_libraries(textfilterplugin PRIVATE KF5::TextEditor)

ki18n_wrap_ui(UI_SOURCES textfilterwidget.ui)
target_sources(textfilterplugin PRIVATE ${UI_SOURCES})

target_sources(
  textfilterplugin
  PRIVATE
    plugin_katetextfilter.cpp
    plugin.qrc
)

kcoreaddons_desktop_to_json(textfilterplugin textfilterplugin.desktop)
install(TARGETS textfilterplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
