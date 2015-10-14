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
)

ki18n_wrap_ui(KATE_APPLICATION_SRCS
    ui/sessionconfigwidget.ui
)

qt5_add_resources( KATE_APPLICATION_SRCS data/kate.qrc )

add_library(kdeinit_kate STATIC ${KATE_APPLICATION_SRCS})
generate_export_header(kdeinit_kate
  EXPORT_FILE_NAME kateprivate_export.h
  EXPORT_MACRO_NAME KATE_TESTS_EXPORT
)

# create executable
add_executable(kate main.cpp)
target_link_libraries(kate kdeinit_kate)

target_link_libraries(kdeinit_kate
PUBLIC
    KF5::TextEditor
    KF5::I18n
    KF5::IconThemes
    KF5::WindowSystem
    KF5::GuiAddons
    KF5::DBusAddons
    KF5::Crash)

if(KF5Activities_FOUND)
    target_link_libraries(kdeinit_kate
    PUBLIC
    KF5::Activities)
endif()

# install executable
install(TARGETS kate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
