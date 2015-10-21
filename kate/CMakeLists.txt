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

# add icons to application sources, to have them bundled
ecm_add_app_icon(KATE_APP_SRCS ICONS ${KATE_ICONS_PNG})

# create executable
add_executable(kate ${KATE_APP_SRCS})
target_link_libraries(kate kdeinit_kate)

# own plist template
set_target_properties (kate PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/data/MacOSXBundleInfo.plist.in)

# install executable
install(TARGETS kate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

# desktop file
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kate.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR})

# appdata
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kate.appdata.xml DESTINATION  ${CMAKE_INSTALL_METAINFODIR})

# default config
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/katerc  DESTINATION  ${CONFIG_INSTALL_DIR})

# install icons
ecm_install_icons(ICONS ${KATE_ICONS_PNG} ${KATE_ICONS_SVG} DESTINATION ${ICON_INSTALL_DIR} THEME hicolor)

# automatic unit tests
ecm_optional_add_subdirectory (autotests)
