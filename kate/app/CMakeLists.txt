########### next target ###############

set(kateinterfaces_LIB_SRCS kateappadaptor.cpp
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
   katemainwindow.cpp
   katepluginmanager.cpp
   kateviewmanager.cpp
   kateviewspace.cpp
   katesavemodifieddialog.cpp
   katemwmodonhddialog.cpp
   katesession.cpp
   katemdi.cpp
   katecontainer.cpp
   katerunninginstanceinfo.cpp
   kateappcommands.cpp
   katequickopen.cpp
   )



kde4_add_library(kateinterfaces SHARED ${kateinterfaces_LIB_SRCS})

# some stuff to have it compile for KDE 4.4
if (NOT KDE_NO_DEPRECATED)
   set (KDE_4_4_LIBS_NEEDED ${KDE4_KUTILS_LIBS})
endif()

target_link_libraries(kateinterfaces
   LINK_PUBLIC ${KDE4_KPARTS_LIBS}
   LINK_PRIVATE ${KDE_4_4_LIBS_NEEDED} ${QT_QTXML_LIBRARY} ${KDE4_KTEXTEDITOR_LIBS} ${KDE4_KPARTS_LIBS} ${KACTIVITIES_LIBRARY} )

set_target_properties(kateinterfaces PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS kateinterfaces ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### next target ###############

set(kate_KDEINIT_SRCS katemain.cpp)

if (WIN32)
    configure_file(${KDE4_MODULE_DIR}/kde4init_dummy.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/kate_dummy.cpp)

    list(APPEND kate_KDEINIT_SRCS ${CMAKE_CURRENT_BINARY_DIR}/kate_dummy.cpp)

    #kde4_add_app_icon(kate_KDEINIT_SRCS "${KDE4_INSTALL_DIR}/share/icons/oxygen/*/apps/kate.png")
    kde4_add_app_icon(kate_KDEINIT_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../pics/hi*-app-kate.png")

    kde4_add_executable(kate ${kate_KDEINIT_SRCS} )

    target_link_libraries( kate ${QT_QTMAIN_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} kateinterfaces )

else (WIN32)
    kde4_add_kdeinit_executable( kate ${kate_KDEINIT_SRCS})

    target_link_libraries(kdeinit_kate ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} kateinterfaces )

    install(TARGETS kdeinit_kate ${INSTALL_TARGETS_DEFAULT_ARGS} )

    target_link_libraries( kate kdeinit_kate )
endif (WIN32)

install(TARGETS kate ${INSTALL_TARGETS_DEFAULT_ARGS})

