project (kwrite)

# Load the frameworks we need
find_package(KF5 REQUIRED COMPONENTS DBusAddons)

set(kwrite_KDEINIT_SRCS main.cpp kwrite.cpp)

kf5_add_kdeinit_executable(kwrite ${kwrite_KDEINIT_SRCS})
target_link_libraries(kdeinit_kwrite
  KF5::TextEditor
  KF5::I18n
  KF5::KIOFileWidgets
  KF5::DBusAddons
  KF5::IconThemes
)

install(TARGETS kwrite         ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS kdeinit_kwrite ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kwrite.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES org.kde.kwrite.appdata.xml DESTINATION ${CMAKE_INSTALL_METAINFODIR} )
install(FILES kwriteui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/kwrite)
