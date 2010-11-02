
project(katefiletreeplugin)

set(katefiletreeplugin_PART_SRCS katefiletreeplugin.cpp katefiletree.cpp katefiletreemodel.cpp katefiletreeproxymodel.cpp katefiletreeconfigpage.cpp katefiletreepluginsettings.cpp )

if(NOT KDE4_FOUND)
  find_package(KDE4 REQUIRED)
  include_directories( ${KDE4_INCLUDES} ${QT_INCLUDES} )
endif(NOT KDE4_FOUND)

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )
kde4_add_plugin(katefiletreeplugin ${katefiletreeplugin_PART_SRCS})

target_link_libraries(katefiletreeplugin ${KDE4_KFILE_LIBS} ${KDE4_KPARTS_LIBS} kateinterfaces ${KDE4_KTEXTEDITOR_LIBS} )

install(TARGETS katefiletreeplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/filetree )
install( FILES katefiletreeplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

