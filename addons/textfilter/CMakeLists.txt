set(textfilterplugin_PART_SRCS
  plugin_katetextfilter.cpp
  plugin.qrc
)

ki18n_wrap_ui(textfilterplugin_PART_SRCS textfilterwidget.ui)

add_library(textfilterplugin MODULE ${textfilterplugin_PART_SRCS})
target_compile_definitions(textfilterplugin PRIVATE TRANSLATION_DOMAIN="katetextfilter")
target_link_libraries(textfilterplugin PRIVATE KF5::TextEditor)

kcoreaddons_desktop_to_json(textfilterplugin textfilterplugin.desktop)
install(TARGETS textfilterplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
