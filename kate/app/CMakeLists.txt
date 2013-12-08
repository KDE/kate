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



add_library (kateinterfaces SHARED ${kateinterfaces_LIB_SRCS})

target_link_libraries (kateinterfaces ktexteditor
    KF5::KParts
    KF5::KDE4Support
    KF5::KNewStuff Qt5::Script KF5::SonnetCore KF5::KArchive KF5::KCMUtils KF5::KPrintUtils KF5::ItemViews
)

set_target_properties(kateinterfaces PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS kateinterfaces ${INSTALL_TARGETS_DEFAULT_ARGS} )

#
# Kate Application
#
kf5_add_kdeinit_executable(kate katemain.cpp)
target_link_libraries(kdeinit_kate ktexteditor kateinterfaces)

install(TARGETS kate         ${INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS kdeinit_kate ${INSTALL_TARGETS_DEFAULT_ARGS})
