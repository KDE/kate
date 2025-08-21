kate_add_plugin(katebuildplugin)
target_compile_definitions(katebuildplugin PRIVATE TRANSLATION_DOMAIN="katebuild-plugin")
target_link_libraries(katebuildplugin PRIVATE KF6::I18n KF6::TextEditor kateprivate)

ki18n_wrap_ui(katebuildplugin build.ui buildconfigwidget.ui)

include(ECMQtDeclareLoggingCategory)
ecm_qt_declare_logging_category(
  DEBUG_SOURCES
  HEADER kate_buildplugin_debug.h
  IDENTIFIER KTEBUILD
  CATEGORY_NAME "katebuildplugin"
)

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
    buildconfig.cpp
    StatusOverlay.cpp
    plugin.qrc
    qcmakefileapi.cpp
    ${DEBUG_SOURCES}
)

if (ENABLE_PCH)
    target_precompile_headers(katebuildplugin REUSE_FROM katepch)
endif()
