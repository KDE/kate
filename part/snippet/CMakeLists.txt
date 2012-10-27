
add_definitions( -DKDE_DEFAULT_DEBUG_AREA=9521 )


########### next target ###############

set(kdevsnippet_PART_SRCS
    snippetplugin.cpp
    snippetview.cpp
    snippetstore.cpp
    snippetrepository.cpp
    snippetfilterproxymodel.cpp
    snippetcompletionmodel.cpp
    snippetcompletionitem.cpp
    snippet.cpp
    editrepository.cpp
    editsnippet.cpp
)
set(kdevsnippet_PART_UI
    snippetview.ui
    editrepository.ui
    editsnippet.ui
)

kde4_add_ui_files(kdevsnippet_PART_SRCS ${kdevsnippet_PART_UI})
kde4_add_plugin(kdevsnippet ${kdevsnippet_PART_SRCS})
target_link_libraries(kdevsnippet ${KDE4_KTEXTEDITOR_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KPARTS_LIBS} ${KDE4_KNEWSTUFF3_LIBS}
kdevplatformlanguage kdevplatforminterfaces)
install( TARGETS kdevsnippet DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############
install( FILES kdevsnippet.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES kdevsnippet.rc DESTINATION ${DATA_INSTALL_DIR}/kdevsnippet )
