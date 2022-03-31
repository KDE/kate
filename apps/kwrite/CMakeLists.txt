# KWrite executable, just main.cpp & resource file & our large shared part in the static library
add_executable(kwrite main.cpp  data/kwrite.qrc)
target_link_libraries(kwrite PRIVATE kate-lib)

# application icon, in all sizes needed to have e.g. nice Windows icons, too
set(ICONS_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/icons)
set(ICON_FILES
    ${ICONS_FOLDER}/sc-apps-kwrite.svg
    ${ICONS_FOLDER}/16-apps-kwrite.png
    ${ICONS_FOLDER}/22-apps-kwrite.png
    ${ICONS_FOLDER}/32-apps-kwrite.png
    ${ICONS_FOLDER}/44-apps-kwrite.png
    ${ICONS_FOLDER}/48-apps-kwrite.png
    ${ICONS_FOLDER}/64-apps-kwrite.png
    ${ICONS_FOLDER}/128-apps-kwrite.png
    ${ICONS_FOLDER}/150-apps-kwrite.png
    ${ICONS_FOLDER}/256-apps-kwrite.png
    ${ICONS_FOLDER}/310-apps-kwrite.png
    ${ICONS_FOLDER}/512-apps-kwrite.png
)

# Add icon files to the application's source files to have CMake bundle them in the executable.
ecm_add_app_icon(ICONS_SOURCES ICONS ${ICON_FILES})
target_sources(kwrite PRIVATE ${ICONS_SOURCES})

# See https://cmake.org/cmake/help/v3.15/prop_tgt/MACOSX_BUNDLE_INFO_PLIST.html
if(APPLE)
  set_property(
    TARGET kwrite
    PROPERTY MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/data/MacOSXBundleInfo.plist.in
  )

  # These are substituted by CMake into plist.in.
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.KWrite")
  set(MACOSX_BUNDLE_BUNDLE_NAME "KWrite")
  set(MACOSX_BUNDLE_DISPLAY_NAME "KWrite")
  set(MACOSX_BUNDLE_INFO_STRING "KWrite - Text Editor")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "KWrite ${RELEASE_SERVICE_VERSION}")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${RELEASE_SERVICE_VERSION}")
  set(MACOSX_BUNDLE_COPYRIGHT "2000-2022 The KWrite Authors")
endif()

install(TARGETS kwrite ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

ecm_install_icons(
    ICONS ${ICON_FILES}
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kwrite.desktop
  DESTINATION ${KDE_INSTALL_APPDIR}
)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kwrite.appdata.xml
  DESTINATION ${KDE_INSTALL_METAINFODIR}
)
