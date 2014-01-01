#
# The Kate Application
#

# collect the needed source files
set (KATE_APPLICATION_SRCS
   kateappadaptor.cpp
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
   katerunninginstanceinfo.cpp
   kateappcommands.cpp
   katequickopen.cpp
   katemain.cpp)

# create kde init executable
kf5_add_kdeinit_executable(kate ${KATE_APPLICATION_SRCS})
target_link_libraries(kdeinit_kate ktexteditor
    KF5::I18n
    KF5::KIOFileWidgets
    KF5::IconThemes
    KF5::WindowSystem
    KF5::GuiAddons
    KF5::DBusAddons)

# install executable + kdeinit lib
install(TARGETS kate         ${INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS kdeinit_kate ${INSTALL_TARGETS_DEFAULT_ARGS})
