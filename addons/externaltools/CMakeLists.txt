project(externaltoolsplugin)
add_definitions(-DTRANSLATION_DOMAIN=\"externaltoolsplugin\")

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(externaltoolsplugin_PART_SRCS
    externaltoolsplugin.cpp
    externaltools.cpp
    katetoolrunner.cpp
    kateexternaltool.cpp
    katemacroexpander.cpp
)

# resource for ui file and stuff
qt5_add_resources(externaltoolsplugin_PART_SRCS plugin.qrc)

set(externaltoolsplugin_PART_UI
    configwidget.ui
    tooldialog.ui
)
ki18n_wrap_ui(externaltoolsplugin_PART_SRCS ${externaltoolsplugin_PART_UI} )
add_library(externaltoolsplugin MODULE ${externaltoolsplugin_PART_SRCS})

# we compile in the .desktop file
kcoreaddons_desktop_to_json (externaltoolsplugin externaltoolsplugin.desktop)

target_link_libraries(externaltoolsplugin
    KF5::CoreAddons
    KF5::IconThemes
    KF5::TextEditor
    KF5::I18n
)

########### install files ###############
install(TARGETS externaltoolsplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )

############# unit tests ################
if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
