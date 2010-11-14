########### next target ###############

set(katefindinfilesplugin_PART_SRCS
    katefindinfiles.cpp
    katefinddialog.cpp
    katefindoptions.cpp
    kateresultview.cpp
    kategrepthread.cpp
)


set( katefindinfilesplugin_PART_UI
    findwidget.ui
    resultwidget.ui
)

kde4_add_ui_files(katefindinfilesplugin_PART_SRCS ${katefindinfilesplugin_PART_UI} )

kde4_add_plugin(katefindinfilesplugin ${katefindinfilesplugin_PART_SRCS})

target_link_libraries(katefindinfilesplugin  ${KDE4_KDECORE_LIBS} ${KDE4_KTEXTEDITOR_LIBS} ${KDE4_KPARTS_LIBS} kateinterfaces )

install(FILES ui.rc DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/findinfiles)
install(TARGETS katefindinfilesplugin  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES katefindinfilesplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
