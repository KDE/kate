# We use an intermediate static library because linking tests directly to an executable is not
# possible with CMake yet.
add_library(kate-lib STATIC "")

configure_file(config.h.in config.h)

include(GenerateExportHeader)
generate_export_header(
  kate-lib
  EXPORT_FILE_NAME katetests_export.h
  EXPORT_MACRO_NAME KATE_TESTS_EXPORT
)

target_include_directories(
  kate-lib
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/session
    ${CMAKE_CURRENT_SOURCE_DIR}/qtsingleapplication
    ${CMAKE_CURRENT_SOURCE_DIR}/quickopen
    ${CMAKE_CURRENT_BINARY_DIR} # katetests_export.h + config.h
)

find_package(
  KF5 ${KF5_DEP_VERSION}
  QUIET
  REQUIRED
  COMPONENTS
    TextEditor
    WindowSystem
    DBusAddons
    Crash
  OPTIONAL_COMPONENTS
    Activities
)

target_link_libraries(
  kate-lib
  PUBLIC
    KF5::I18n
    KF5::TextEditor
    KF5::WindowSystem
    KF5::DBusAddons
    KF5::Crash
    KF5::TextWidgets
    kateshared
)

if(KF5Activities_FOUND)
  target_link_libraries(kate-lib PUBLIC KF5::Activities)
endif()

# optional KUserFeedback integration
find_package(KUserFeedback)
if (TARGET KUserFeedbackWidgets)
    target_link_libraries(kate-lib PUBLIC KUserFeedbackWidgets)
    target_compile_definitions(kate-lib PUBLIC -DWITH_KUSERFEEDBACK)
endif()

ki18n_wrap_ui(kate-lib ui/sessionconfigwidget.ui session/katesessionmanagedialog.ui)

target_sources(
  kate-lib
  PRIVATE
    session/katesession.cpp
    session/katesessionmanagedialog.cpp
    session/katesessionmanager.cpp
    session/katesessionsaction.cpp

    quickopen/katequickopen.cpp
    quickopen/katequickopenmodel.cpp
    quickopen/katequickopenlineedit.cpp

    kateapp.cpp
    kateappadaptor.cpp
    kateconfigdialog.cpp
    kateconfigplugindialogpage.cpp
    katedocmanager.cpp
    katefileactions.cpp
    katemainwindow.cpp
    katemdi.cpp
    katemwmodonhddialog.cpp
    katepluginmanager.cpp

    katerunninginstanceinfo.cpp
    katesavemodifieddialog.cpp
    katetabbar.cpp
    katesplitter.cpp
    kateviewmanager.cpp
    kateviewspace.cpp
    katewaiter.cpp
    tabmimedata.cpp

    kateoutputview.cpp
    katestashmanager.cpp

    kateurlbar.cpp

    signal_watcher.cpp
)

# Use a single application on MacOS + Windows instead of dbus.
if(APPLE OR WIN32)
  target_sources(
    kate-lib
    PRIVATE
      qtsingleapplication/qtlocalpeer.cpp
      qtsingleapplication/qtsingleapplication.cpp
      qtsingleapplication/qtlockedfile.cpp
  )

  if(WIN32)
    target_sources(kate-lib PRIVATE qtsingleapplication/qtlockedfile_win.cpp)
  else()
    target_sources(kate-lib PRIVATE qtsingleapplication/qtlockedfile_unix.cpp)
  endif()
endif()

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
