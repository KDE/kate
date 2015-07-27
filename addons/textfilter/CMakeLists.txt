project(textfilterplugin)
add_definitions(-DTRANSLATION_DOMAIN=\"katetextfilter\")

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(textfilterplugin_PART_SRCS
    plugin_katetextfilter.cpp
)

ki18n_wrap_ui(textfilterplugin_PART_SRCS textfilterwidget.ui)

add_library (textfilterplugin MODULE ${textfilterplugin_PART_SRCS})

target_link_libraries(textfilterplugin
    KF5::TextEditor
    KF5::IconThemes
    KF5::I18n
    KF5::Service
)

install(TARGETS textfilterplugin  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

########### install files ###############

install(FILES ui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/textfilter)

kcoreaddons_desktop_to_json (textfilterplugin textfilterplugin.desktop)
