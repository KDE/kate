#
# The Kate Application
#
project(kate)

# Load the frameworks we need
find_package(KF5 CONFIG REQUIRED COMPONENTS DBusAddons)

# .desktop files and co
add_subdirectory (data)

# icons
add_subdirectory (icons)

# includes
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/session
)

# collect the needed source files
set (KATE_APPLICATION_SRCS
   src/kateappadaptor.cpp
   src/kateapp.cpp
   src/kateconfigdialog.cpp
   src/kateconfigplugindialogpage.cpp
   src/katedocmanager.cpp
   src/katemainwindow.cpp
   src/katepluginmanager.cpp
   src/kateviewmanager.cpp
   src/kateviewspace.cpp
   src/katesavemodifieddialog.cpp
   src/katemwmodonhddialog.cpp

   src/katetabbutton.cpp
   src/katetabbar.cpp

   # session
   src/session/katesessionchooser.cpp
   src/session/katesessionsaction.cpp
   src/session/katesessionmanager.cpp
   src/session/katesessionmanagedialog.cpp
   src/session/katesessionopendialog.cpp
   src/session/katesession.cpp

   src/katemdi.cpp
   src/katerunninginstanceinfo.cpp
   src/kateappcommands.cpp
   src/katequickopen.cpp
   src/katewaiter.h
   src/main.cpp
)

qt5_wrap_ui(KATE_APPLICATION_SRCS
    src/ui/sessionconfigwidget.ui
)

# create kde init executable
kf5_add_kdeinit_executable(kate ${KATE_APPLICATION_SRCS})
target_link_libraries(kdeinit_kate
    KF5::TextEditor
    KF5::I18n
    KF5::KIOFileWidgets
    KF5::IconThemes
    KF5::WindowSystem
    KF5::GuiAddons
    KF5::DBusAddons)

generate_export_header(kdeinit_kate
  EXPORT_FILE_NAME kateprivate_export.h
  EXPORT_MACRO_NAME KATE_TESTS_EXPORT
)

# install executable + kdeinit lib
install(TARGETS kate         ${INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS kdeinit_kate ${INSTALL_TARGETS_DEFAULT_ARGS})

# automatic unit tests
ecm_optional_add_subdirectory (autotests)
