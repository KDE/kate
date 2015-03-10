# includes
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/session
)

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

   katetabbutton.cpp
   katetabbar.cpp

   # session
   session/katesessionchooser.cpp
   session/katesessionsaction.cpp
   session/katesessionmanager.cpp
   session/katesessionmanagedialog.cpp
   session/katesessionopendialog.cpp
   session/katesession.cpp

   katemdi.cpp
   katerunninginstanceinfo.cpp
   katequickopen.cpp
   katewaiter.h
   main.cpp
)

ki18n_wrap_ui(KATE_APPLICATION_SRCS
    ui/sessionconfigwidget.ui
)

qt5_add_resources( KATE_APPLICATION_SRCS data/kate.qrc )

# create kde init executable
kf5_add_kdeinit_executable(kate ${KATE_APPLICATION_SRCS})
target_link_libraries(kdeinit_kate
PUBLIC
    KF5::TextEditor
    KF5::I18n
    KF5::KIOFileWidgets
    KF5::IconThemes
    KF5::WindowSystem
    KF5::GuiAddons
    KF5::DBusAddons
    KF5::Crash)

generate_export_header(kdeinit_kate
  EXPORT_FILE_NAME kateprivate_export.h
  EXPORT_MACRO_NAME KATE_TESTS_EXPORT
)

# install executable + kdeinit lib
install(TARGETS kate         ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS kdeinit_kate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
