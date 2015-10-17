project(katekonsoleplugin)
add_definitions(-DTRANSLATION_DOMAIN=\"katekonsoleplugin\")

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(katekonsoleplugin_PART_SRCS kateconsole.cpp )

# resource for ui file and stuff
qt5_add_resources(katekonsoleplugin_PART_SRCS plugin.qrc)

add_library (katekonsoleplugin MODULE ${katekonsoleplugin_PART_SRCS})

kcoreaddons_desktop_to_json (katekonsoleplugin katekonsoleplugin.desktop)

target_link_libraries(katekonsoleplugin
    KF5::TextEditor
    KF5::Parts
    KF5::IconThemes
    KF5::I18n
    KF5::Service
)

install(TARGETS katekonsoleplugin  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )
