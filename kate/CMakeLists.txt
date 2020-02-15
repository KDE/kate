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
    ${CMAKE_CURRENT_BINARY_DIR} # katetests_export.h + config.h
)

find_package(
  KF5 ${KF5_DEP_VERSION}
  QUIET
  REQUIRED
  COMPONENTS
    TextEditor
    IconThemes
    WindowSystem
    DBusAddons
    Crash
  OPTIONAL_COMPONENTS
    Activities
)

target_link_libraries(
  kate-lib
  PUBLIC
    KF5::TextEditor
    KF5::IconThemes
    KF5::WindowSystem
    KF5::DBusAddons
    KF5::Crash
)

if(KF5Activities_FOUND)
  target_link_libraries(kate-lib PUBLIC KF5::Activities)
endif()

ki18n_wrap_ui(UI_SOURCES ui/sessionconfigwidget.ui session/katesessionmanagedialog.ui)
target_sources(kate-lib PRIVATE ${UI_SOURCES})

target_sources(
  kate-lib
  PRIVATE
    data/kate.qrc

    session/katesession.cpp
    session/katesessionmanagedialog.cpp
    session/katesessionmanager.cpp
    session/katesessionsaction.cpp

    kateapp.cpp
    kateappadaptor.cpp
    katecolorschemechooser.cpp
    kateconfigdialog.cpp
    kateconfigplugindialogpage.cpp
    katedocmanager.cpp
    katefileactions.cpp
    katemainwindow.cpp
    katemdi.cpp
    katemwmodonhddialog.cpp
    katepluginmanager.cpp
    katequickopen.cpp
    katequickopenmodel.cpp
    katerunninginstanceinfo.cpp
    katesavemodifieddialog.cpp
    katetabbar.cpp
    katetabbutton.cpp
    kateviewmanager.cpp
    kateviewspace.cpp
    katewaiter.cpp
)

# Executable only adds the main definition.
add_executable(kate-bin main.cpp)
target_link_libraries(kate-bin PRIVATE kate-lib)

# Use a single application on MacOS + Windows instead of dbus.
if(APPLE OR WIN32)
  target_compile_definitions(kate-bin PRIVATE USE_QT_SINGLE_APP)

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

# application icon, in all sizes needed to have e.g. nice Windows icons, too
set(ICON_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/sc-apps-kate.svg
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/16-apps-kate.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/22-apps-kate.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/32-apps-kate.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/44-apps-kate.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/48-apps-kate.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/64-apps-kate.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/128-apps-kate.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/150-apps-kate.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/256-apps-kate.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/310-apps-kate.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/512-apps-kate.png
)

# Add icon files to the application's source files to have CMake bundle them in the executable.
ecm_add_app_icon(ICONS_SOURCES ICONS ${ICON_FILES})
target_sources(kate-bin PRIVATE ${ICONS_SOURCES})

set_property(
  TARGET kate-bin
  PROPERTY OUTPUT_NAME kate
)

# See https://cmake.org/cmake/help/v3.15/prop_tgt/MACOSX_BUNDLE_INFO_PLIST.html
if(APPLE)
  set_property(
    TARGET kate-bin
    PROPERTY MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/data/MacOSXBundleInfo.plist.in
  )

  # These are substituted by CMake into plist.in.
  set(MACOSX_BUNDLE_DISPLAY_NAME "Kate")
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Kate")
  set(MACOSX_BUNDLE_BUNDLE_NAME "Kate")
  set(MACOSX_BUNDLE_DISPLAY_NAME "Kate")
  set(MACOSX_BUNDLE_INFO_STRING "Kate - Advanced Text Editor")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "Kate ${KDE_APPLICATIONS_VERSION}")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${KDE_APPLICATIONS_VERSION}")
  set(MACOSX_BUNDLE_COPYRIGHT "2000-2016 The Kate Authors")
endif()

install(TARGETS kate-bin ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

ecm_install_icons(
  ICONS ${ICON_FILES}
  DESTINATION ${ICON_INSTALL_DIR}
  THEME hicolor
)

install(
  PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kate.desktop
  DESTINATION ${KDE_INSTALL_APPDIR}
)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kate.appdata.xml
  DESTINATION ${CMAKE_INSTALL_METAINFODIR}
)

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
