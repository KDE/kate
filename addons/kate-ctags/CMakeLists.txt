add_definitions(-DTRANSLATION_DOMAIN=\"kate-ctags-plugin\")

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(ctagsplugin_SRC
    readtags.c
    tags.cpp
    ctagskinds.cpp
    kate_ctags_view.cpp
    kate_ctags_plugin.cpp
)

ki18n_wrap_ui(ctagsplugin_SRC kate_ctags.ui CTagsGlobalConfig.ui)

# resource for ui file and stuff
qt5_add_resources(ctagsplugin_SRC plugin.qrc)

add_library(katectagsplugin MODULE ${ctagsplugin_SRC})

kcoreaddons_desktop_to_json (katectagsplugin katectagsplugin.desktop)

target_link_libraries(katectagsplugin KF5::TextEditor KF5::I18n KF5::IconThemes)

install(TARGETS katectagsplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )
