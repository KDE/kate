include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(ctagsplugin_SRC
    readtags.c
    tags.cpp
    ctagskinds.cpp
    kate_ctags_view.cpp
    kate_ctags_plugin.cpp
)

qt5_wrap_ui(ctagsplugin_SRC kate_ctags.ui CTagsGlobalConfig.ui)

add_library(katectagsplugin MODULE ${ctagsplugin_SRC})

target_link_libraries(katectagsplugin KF5::TextEditor KF5::I18n KF5::IconThemes)

########### install files ###############
install(TARGETS katectagsplugin DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES ui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/katectags)

kservice_desktop_to_json (katectagsplugin katectagsplugin.desktop)
install(FILES katectagsplugin.desktop DESTINATION  ${SERVICES_INSTALL_DIR} )
