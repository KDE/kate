# Kate application
add_executable(kate-bin
    ${CMAKE_SOURCE_DIR}/3rdparty/SingleApplication/singleapplication.cpp
    ${CMAKE_SOURCE_DIR}/3rdparty/SingleApplication/singleapplication_p.cpp
    main.cpp
    data/kate.qrc)

# we use SingleApplication without letting it inherit from QApplication
target_compile_definitions(kate-bin PRIVATE -DQAPPLICATION_CLASS=FreeStandingSingleApplication)

# add dbus support class if needed
if(WITH_DBUS)
    target_sources(kate-bin PRIVATE katewaiter.cpp)
endif()

target_link_libraries(kate-bin PRIVATE kateprivate)

# X11 startup handling
if (UNIX AND NOT APPLE)
    target_link_libraries(kate-bin PRIVATE Qt::GuiPrivate)
endif()

# ensure right translations domain, we use one kate.po(t) for lib + apps
target_compile_definitions(kate-bin PRIVATE TRANSLATION_DOMAIN="kate")

# we have different windows icons, less margins
set(ICONS_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/icons/unix)
if(WIN32)
    set(ICONS_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/icons/windows)
endif()

# application icon, in all sizes needed to have e.g. nice Windows icons, too
set(ICON_FILES
  ${ICONS_FOLDER}/sc-apps-kate.svg
  ${ICONS_FOLDER}/16-apps-kate.png
  ${ICONS_FOLDER}/22-apps-kate.png
  ${ICONS_FOLDER}/32-apps-kate.png
  ${ICONS_FOLDER}/44-apps-kate.png
  ${ICONS_FOLDER}/48-apps-kate.png
  ${ICONS_FOLDER}/64-apps-kate.png
  ${ICONS_FOLDER}/128-apps-kate.png
  ${ICONS_FOLDER}/150-apps-kate.png
  ${ICONS_FOLDER}/256-apps-kate.png
  ${ICONS_FOLDER}/310-apps-kate.png
  ${ICONS_FOLDER}/512-apps-kate.png
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
  # we need to copy the file to avoid issues with duplicate generation targets
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../lib/data/MacOSXBundleInfo.plist.in ${CMAKE_CURRENT_BINARY_DIR}/plist.in COPYONLY)
  set_property(
    TARGET kate-bin
    PROPERTY MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/plist.in
  )

  # These are substituted by CMake into plist.in.
  set(MACOSX_BUNDLE_DISPLAY_NAME "Kate")
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Kate")
  set(MACOSX_BUNDLE_BUNDLE_NAME "Kate")
  set(MACOSX_BUNDLE_DISPLAY_NAME "Kate")
  set(MACOSX_BUNDLE_INFO_STRING "Kate - Advanced Text Editor")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "Kate ${RELEASE_SERVICE_VERSION}")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${RELEASE_SERVICE_VERSION}")
  set(MACOSX_BUNDLE_COPYRIGHT "2000-2025 The Kate Authors")
endif()

install(TARGETS kate-bin ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

ecm_install_icons(
  ICONS ${ICON_FILES}
  DESTINATION ${KDE_INSTALL_ICONDIR}
  THEME hicolor
)

install(
  PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kate.desktop
  DESTINATION ${KDE_INSTALL_APPDIR}
)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kate.appdata.xml
  DESTINATION ${KDE_INSTALL_METAINFODIR}
)
