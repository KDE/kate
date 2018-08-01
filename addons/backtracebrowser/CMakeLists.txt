########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"katebacktracebrowserplugin\")

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(katebacktracebrowserplugin_PART_SRCS
    katebacktracebrowser.cpp
    btparser.cpp
    btfileindexer.cpp
    btdatabase.cpp
)

set(katebacktracebrowserplugin_PART_UI
    btbrowserwidget.ui
    btconfigwidget.ui
)
ki18n_wrap_ui(katebacktracebrowserplugin_PART_SRCS ${katebacktracebrowserplugin_PART_UI} )
add_library(katebacktracebrowserplugin MODULE ${katebacktracebrowserplugin_PART_SRCS})

# we compile in the .desktop file
kcoreaddons_desktop_to_json (katebacktracebrowserplugin katebacktracebrowserplugin.desktop)

target_link_libraries(katebacktracebrowserplugin
    KF5::TextEditor
    KF5::I18n
)

########### install files ###############
install( TARGETS katebacktracebrowserplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )

############# unit tests ################
if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
