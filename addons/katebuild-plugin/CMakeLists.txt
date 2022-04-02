kate_add_plugin(katebuildplugin)
target_compile_definitions(katebuildplugin PRIVATE TRANSLATION_DOMAIN="katebuild-plugin")
target_link_libraries(katebuildplugin PRIVATE KF5::I18n KF5::TextEditor)

ki18n_wrap_ui(katebuildplugin build.ui)

target_sources(
  katebuildplugin
  PRIVATE
    plugin_katebuild.cpp
    targets.cpp
    TargetHtmlDelegate.cpp
    TargetModel.cpp
    UrlInserter.cpp
    TargetFilterProxyModel.cpp
    plugin.qrc
)

target_include_directories(
    katebuildplugin
    PUBLIC
    ${CMAKE_SOURCE_DIR}/shared
)
