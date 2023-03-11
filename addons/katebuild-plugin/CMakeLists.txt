kate_add_plugin(katebuildplugin)
target_compile_definitions(katebuildplugin PRIVATE TRANSLATION_DOMAIN="katebuild-plugin")
target_link_libraries(katebuildplugin PRIVATE KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::TextEditor kateprivate)

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
    AppOutput.cpp
    plugin.qrc
)

if (BUILD_PCH)
    target_precompile_headers(katebuildplugin REUSE_FROM katepch)
endif()
