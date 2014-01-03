project (kwrite)

# Load the frameworks we need
find_package(KF5 CONFIG REQUIRED COMPONENTS DBusAddons)

set(kwrite_KDEINIT_SRCS kwritemain.cpp kwriteapp.cpp)

# FIXME KF5
#kde4_add_app_icon(kwrite_KDEINIT_SRCS "${KDE4_INSTALL_DIR}/share/icons/oxygen/*/apps/accessories-text-editor.png")

kf5_add_kdeinit_executable(kwrite ${kwrite_KDEINIT_SRCS})
target_link_libraries(kdeinit_kwrite ktexteditor
  KF5::I18n
  KF5::KIOFileWidgets
  KF5::DBusAddons
  KF5::IconThemes
)

install(TARGETS kwrite         ${INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS kdeinit_kwrite ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS kwrite.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES kwriteui.rc DESTINATION ${DATA_INSTALL_DIR}/kwrite)
