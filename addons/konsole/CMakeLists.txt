project(katekonsoleplugin)

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(katekonsoleplugin_PART_SRCS kateconsole.cpp )

add_library (katekonsoleplugin MODULE ${katekonsoleplugin_PART_SRCS})

target_link_libraries(katekonsoleplugin
    KF5::TextEditor
    KF5::Parts
    KF5::IconThemes
    KF5::I18n
    KF5::Service
)

install(TARGETS katekonsoleplugin  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )

########### install files ###############

install( FILES ui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/katekonsole )

kcoreaddons_desktop_to_json (katekonsoleplugin katekonsoleplugin.desktop)
install( FILES katekonsoleplugin.desktop DESTINATION  ${SERVICES_INSTALL_DIR} )

