project(kateexternaltoolsplugin)
add_definitions(-DTRANSLATION_DOMAIN=\"externaltoolsplugin\")

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(kateexternaltoolsplugin_PART_SRCS
    kateexternaltoolsplugin.cpp
    kateexternaltools.cpp
)

# resource for ui file and stuff
qt5_add_resources(kateexternaltoolsplugin_PART_SRCS plugin.qrc)

set(kateexternaltoolsplugin_PART_UI
    configwidget.ui
)
ki18n_wrap_ui(kateexternaltoolsplugin_PART_SRCS ${katebacktracebrowserplugin_PART_UI} )
add_library(kateexternaltoolsplugin MODULE ${kateexternaltoolsplugin_PART_SRCS})

# we compile in the .desktop file
kcoreaddons_desktop_to_json (kateexternaltoolsplugin kateexternaltoolsplugin.desktop)

target_link_libraries(kateexternaltoolsplugin
    KF5::TextEditor
    KF5::I18n
)

########### install files ###############
install(TARGETS kateexternaltoolsplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )
