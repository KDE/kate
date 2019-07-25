set(katebuild_SRCS
  plugin_katebuild.cpp
  targets.cpp
  TargetHtmlDelegate.cpp
  TargetModel.cpp
  UrlInserter.cpp
  SelectTargetView.cpp
  plugin.qrc
)

ki18n_wrap_ui(katebuild_SRCS build.ui SelectTargetUi.ui)

add_library(katebuildplugin MODULE ${katebuild_SRCS})
target_compile_definitions(katebuildplugin PRIVATE TRANSLATION_DOMAIN="katebuild-plugin")

target_link_libraries(
  katebuildplugin 
  PRIVATE 
    KF5::TextEditor 
    KF5::I18n 
    KF5::IconThemes
)

kcoreaddons_desktop_to_json (katebuildplugin katebuildplugin.desktop)
install(TARGETS katebuildplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
