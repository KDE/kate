# Load the frameworks we need
find_package(KF5 REQUIRED COMPONENTS Notifications)

add_definitions(-DTRANSLATION_DOMAIN=\"katebuild-plugin\")

set(katebuild_SRCS plugin_katebuild.cpp targets.cpp selecttargetdialog.cpp)

ki18n_wrap_ui(katebuild_SRCS build.ui)

add_library(katebuildplugin MODULE ${katebuild_SRCS})

target_link_libraries(katebuildplugin KF5::TextEditor KF5::I18n KF5::IconThemes KF5::Notifications)

########### install files ###############
install(FILES ui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/katebuild)
install(TARGETS katebuildplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
install(FILES katebuildplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})
ecm_install_icons (${ICON_INSTALL_DIR})
kcoreaddons_desktop_to_json (katebuildplugin katebuildplugin.desktop)
