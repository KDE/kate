# KWrite
project (kwrite)

# Load the frameworks we need
find_package(KF5 REQUIRED COMPONENTS DBusAddons)

# collect the sources
set (KWRITE_APPLICATION_SRCS main.cpp kwrite.cpp)
qt5_add_resources(KWRITE_APPLICATION_SRCS kwrite.qrc)

# build KWrite application
kf5_add_kdeinit_executable(kwrite ${KWRITE_APPLICATION_SRCS})
target_link_libraries(kdeinit_kwrite
PUBLIC
  KF5::TextEditor
  KF5::I18n
  KF5::DBusAddons
  KF5::Crash
)

if(KF5Activities_FOUND)
    target_link_libraries(kdeinit_kwrite
    PUBLIC
    KF5::Activities)
endif()

# install all parts of KWrite
install(TARGETS kwrite         ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS kdeinit_kwrite ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kwrite.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES org.kde.kwrite.appdata.xml DESTINATION ${CMAKE_INSTALL_METAINFODIR} )
