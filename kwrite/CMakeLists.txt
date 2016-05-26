# KWrite
project (kwrite)

# Load the frameworks we need
find_package(KF5 REQUIRED COMPONENTS DBusAddons)

# collect icons
set(KWRITE_ICONS_PNG
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/16-apps-kwrite.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/22-apps-kwrite.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/32-apps-kwrite.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/48-apps-kwrite.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/64-apps-kwrite.png
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/128-apps-kwrite.png
)

set(KWRITE_ICONS_SVG
${CMAKE_CURRENT_SOURCE_DIR}/icons/sc-apps-kwrite.svgz
)

# collect the sources
set (KWRITE_APPLICATION_SRCS main.cpp kwrite.cpp)
qt5_add_resources(KWRITE_APPLICATION_SRCS data/kwrite.qrc)

# add icons to application sources, to have them bundled
ecm_add_app_icon(KWRITE_APPLICATION_SRCS ICONS ${KWRITE_ICONS_PNG})

# build KWrite application
add_executable(kwrite ${KWRITE_APPLICATION_SRCS})
target_link_libraries(kwrite
PUBLIC
  KF5::TextEditor
  KF5::I18n
  KF5::DBusAddons
  KF5::Crash
)

if(KF5Activities_FOUND)
    target_link_libraries(kwrite
    PUBLIC
    KF5::Activities)
endif()

# own plist magic for mac os
if(APPLE)
    # own plist template
    set_target_properties (kwrite PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/data/MacOSXBundleInfo.plist.in)

    # the MacOSX bundle display name property (CFBundleDisplayName) is not currently supported by cmake,
    # so has to be set for all targets in this cmake file
    set(MACOSX_BUNDLE_DISPLAY_NAME KWrite)
    set_target_properties(kwrite PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.KWrite")
    set_target_properties(kwrite PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KWrite")
    set_target_properties(kwrite PROPERTIES MACOSX_BUNDLE_DISPLAY_NAME "KWrite")
    set_target_properties(kwrite PROPERTIES MACOSX_BUNDLE_INFO_STRING "KWrite - Text Editor")
    set_target_properties(kwrite PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING "KWrite ${KDE_APPLICATIONS_VERSION}")
    set_target_properties(kwrite PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}")
    set_target_properties(kwrite PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION "${KDE_APPLICATIONS_VERSION}")
    set_target_properties(kwrite PROPERTIES MACOSX_BUNDLE_COPYRIGHT "2000-2016 The KWrite Authors")
endif()

# install executable
install(TARGETS kwrite ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

# desktop file
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kwrite.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR})

# appdata
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kwrite.appdata.xml DESTINATION  ${CMAKE_INSTALL_METAINFODIR})

# install icons
ecm_install_icons(ICONS ${KWRITE_ICONS_PNG} ${KWRITE_ICONS_SVG} DESTINATION ${ICON_INSTALL_DIR} THEME hicolor)
