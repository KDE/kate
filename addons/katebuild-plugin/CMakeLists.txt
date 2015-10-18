add_definitions(-DTRANSLATION_DOMAIN=\"katebuild-plugin\")

set(katebuild_SRCS
    plugin_katebuild.cpp
    targets.cpp
    TargetHtmlDelegate.cpp
    TargetModel.cpp
    UrlInserter.cpp
    SelectTargetView.cpp
)

ki18n_wrap_ui(katebuild_SRCS build.ui SelectTargetUi.ui)

# resource for ui file and stuff
qt5_add_resources(katebuild_SRCS plugin.qrc)

add_library(katebuildplugin MODULE ${katebuild_SRCS})
kcoreaddons_desktop_to_json (katebuildplugin katebuildplugin.desktop)
target_link_libraries(katebuildplugin KF5::TextEditor KF5::I18n KF5::IconThemes)

install(TARGETS katebuildplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

