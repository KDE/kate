

########### next target ###############

set(katesnippetsplugin_PART_SRCS katesnippets.cpp )


add_library (katesnippetsplugin MODULE ${katesnippetsplugin_PART_SRCS})

target_link_libraries(katesnippetsplugin ktexteditor
    KF5::Parts KF5::I18n
    KF5::NewStuff Qt5::Script KF5::SonnetCore KF5::Archive KF5::KCMUtils KF5::PrintUtils KF5::ItemViews KF5::IconThemes)

install(TARGETS katesnippetsplugin DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

kservice_desktop_to_json (katesnippetsplugin.desktop)
install( FILES katesnippetsplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
