#
# The Kate Application
#
project(kate)

# Load the frameworks we need
find_package(KF5 REQUIRED COMPONENTS DBusAddons GuiAddons)

# includes
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/session
  ${CMAKE_CURRENT_SOURCE_DIR}/qtsingleapplication
)

# collect the needed source files
set (KATE_LIBRARY_SRCS
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
   katecolorschemechooser.cpp

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

ki18n_wrap_ui(KATE_LIBRARY_SRCS
    ui/sessionconfigwidget.ui
)

qt5_add_resources( KATE_LIBRARY_SRCS data/kate.qrc )

add_library(kdeinit_kate STATIC ${KATE_LIBRARY_SRCS})
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

generate_export_header(kdeinit_kate
  EXPORT_FILE_NAME kateprivate_export.h
  EXPORT_MACRO_NAME KATE_TESTS_EXPORT
)

# collect icons
set(KATE_ICONS_PNG
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/16-apps-kate.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/22-apps-kate.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/32-apps-kate.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/48-apps-kate.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/64-apps-kate.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/128-apps-kate.png
)

set(KATE_ICONS_SVG
${CMAKE_CURRENT_SOURCE_DIR}/icons/sc-apps-kate.svgz
)

# application only sources
set (KATE_APP_SRCS
    main.cpp
)

# use single application instead of dbus on mac + windows
if (APPLE OR WIN32)
    set(singleapp_SRCS
        qtsingleapplication/qtlocalpeer.cpp
        qtsingleapplication/qtsingleapplication.cpp
        qtsingleapplication/qtlockedfile.cpp
    )

    if(WIN32)
        set(singleapp_SRCS ${singleapp_SRCS} qtsingleapplication/qtlockedfile_win.cpp)
    else()
        set(singleapp_SRCS ${singleapp_SRCS} qtsingleapplication/qtlockedfile_unix.cpp)
    endif()

    add_definitions("-DUSE_QT_SINGLE_APP")

    set(KATE_APP_SRCS ${KATE_APP_SRCS} ${singleapp_SRCS})
endif()

# add icons to application sources, to have them bundled
ecm_add_app_icon(KATE_APP_SRCS ICONS ${KATE_ICONS_PNG})

# create executable
add_executable(kate ${KATE_APP_SRCS})
target_link_libraries(kate kdeinit_kate)

# own plist magic for mac os
if(APPLE)
    # own plist template
    set_target_properties (kate PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/data/MacOSXBundleInfo.plist.in)

    # the MacOSX bundle display name property (CFBundleDisplayName) is not currently supported by cmake,
    # so has to be set for all targets in this cmake file
    set(MACOSX_BUNDLE_DISPLAY_NAME Kate)
    set_target_properties(kate PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Kate")
    set_target_properties(kate PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Kate")
    set_target_properties(kate PROPERTIES MACOSX_BUNDLE_DISPLAY_NAME "Kate")
    set_target_properties(kate PROPERTIES MACOSX_BUNDLE_INFO_STRING "Kate - Advanced Text Editor")
    set_target_properties(kate PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING "Kate ${KDE_APPLICATIONS_VERSION}")
    set_target_properties(kate PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}")
    set_target_properties(kate PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION "${KDE_APPLICATIONS_VERSION}")
    set_target_properties(kate PROPERTIES MACOSX_BUNDLE_COPYRIGHT "2000-2016 The Kate Authors")
endif()

# install executable
install(TARGETS kate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

# desktop file
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kate.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR})

# appdata
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kate.appdata.xml DESTINATION  ${CMAKE_INSTALL_METAINFODIR})

# install icons
ecm_install_icons(ICONS ${KATE_ICONS_PNG} ${KATE_ICONS_SVG} DESTINATION ${ICON_INSTALL_DIR} THEME hicolor)

# automatic unit tests
if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
