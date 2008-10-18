find_package(KDE4 REQUIRED)

include(KDE4Defaults)

include_directories (${KDE4_INCLUDES})
include_directories( ${KDE4_KIO_INCLUDES} )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(ctagsplugin_SRC
    readtags.c
    tags.cpp
    ctagskinds.cpp
    kate_ctags.cpp
)

kde4_add_ui_files(ctagsplugin_SRC kate_ctags.ui)

kde4_add_plugin(katectagsplugin ${ctagsplugin_SRC})

target_link_libraries(katectagsplugin ${KDE4_KDECORE_LIBS} kateinterfaces ktexteditor )

install(TARGETS katectagsplugin DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install(FILES ui.rc DESTINATION ${DATA_INSTALL_DIR}/kate/plugins/katectags)
install(FILES katectagsplugin.desktop DESTINATION  ${SERVICES_INSTALL_DIR} )
