########### next target ###############

set(kateinterfaces_LIB_SRCS kateappadaptor.cpp katemainwindowadaptor.cpp
   ../interfaces/kate/application.cpp
   ../interfaces/kate/mainwindow.cpp
   ../interfaces/kate/documentmanager.cpp
   ../interfaces/kate/pluginmanager.cpp
   ../interfaces/kate/plugin.cpp
   ../interfaces/kate/pluginconfigpageinterface.cpp
   kateapp.cpp
   kateconfigdialog.cpp
   kateconfigplugindialogpage.cpp
   katedocmanager.cpp
   katefilelist.cpp
   katemainwindow.cpp
   katepluginmanager.cpp
   kateviewmanager.cpp
   kateviewspace.cpp
   katesavemodifieddialog.cpp
   katemwmodonhddialog.cpp
   katesession.cpp
   katemdi.cpp
   katedocmanageradaptor.cpp
   kateviewdocumentproxymodel.cpp
   )

kde4_automoc(${kateinterfaces_LIB_SRCS})


kde4_add_library(kateinterfaces SHARED ${kateinterfaces_LIB_SRCS})

target_link_libraries(kateinterfaces ${KDE4_KDECORE_LIBS} ${KDE4_KUTILS_LIBS} ${KDE4_KDE3SUPPORT_LIBS}  ${QT_QTXML_LIBRARY} ${KDE4_KTEXTEDITOR_LIBS} )

set_target_properties(kateinterfaces PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS kateinterfaces  DESTINATION ${LIB_INSTALL_DIR} )


########### next target ###############

set(kate_KDEINIT_SRCS katemain.cpp)

kde4_automoc(${kate_KDEINIT_SRCS})

kde4_add_kdeinit_executable( kate ${kate_KDEINIT_SRCS})

target_link_libraries(kdeinit_kate  ${KDE4_KDECORE_LIBS} kateinterfaces )

install(TARGETS kdeinit_kate  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kate kdeinit_kate )
install(TARGETS kate DESTINATION ${BIN_INSTALL_DIR})

