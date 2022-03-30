# KWrite executable, just main.cpp & resource file & our large shared part in the static library
add_executable(kwrite main.cpp  data/kwrite.qrc)
target_link_libraries(kwrite PRIVATE kate-lib)

set(ICONS_PNG
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/16-apps-kwrite.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/22-apps-kwrite.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/32-apps-kwrite.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/48-apps-kwrite.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/64-apps-kwrite.png
  ${CMAKE_CURRENT_SOURCE_DIR}/icons/128-apps-kwrite.png
)

set(ICONS_SVG ${CMAKE_CURRENT_SOURCE_DIR}/icons/sc-apps-kwrite.svgz)

# Add icon files to the application's source files to have CMake bundle them in the executable.
ecm_add_app_icon(ICONS_SOURCES ICONS ${ICONS_PNG})
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

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kwrite.desktop
  DESTINATION ${KDE_INSTALL_APPDIR}
)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kwrite.appdata.xml
  DESTINATION ${KDE_INSTALL_METAINFODIR}
)

ecm_install_icons(
  ICONS ${ICONS_PNG} ${ICONS_SVG}
  DESTINATION ${KDE_INSTALL_ICONDIR}
  THEME hicolor
)
