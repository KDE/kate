kcoreaddons_add_plugin(katebuildplugin INSTALL_NAMESPACE "ktexteditor")
target_compile_definitions(katebuildplugin PRIVATE TRANSLATION_DOMAIN="katebuild-plugin")
target_link_libraries(katebuildplugin PRIVATE KF5::I18n KF5::TextEditor)

ki18n_wrap_ui(UI_SOURCES build.ui SelectTargetUi.ui)
target_sources(katebuildplugin PRIVATE ${UI_SOURCES})

target_sources(
  katebuildplugin
  PRIVATE
    plugin_katebuild.cpp
    targets.cpp
    TargetHtmlDelegate.cpp
    TargetModel.cpp
    UrlInserter.cpp
    SelectTargetView.cpp
    plugin.qrc
)

