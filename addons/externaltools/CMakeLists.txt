find_package(KF6I18n ${KF6_DEP_VERSION} QUIET)
if(NOT KF6I18n_FOUND)
    return()
endif()

find_package(KF6IconThemes ${KF6_DEP_VERSION} QUIET)
if(NOT KF6IconThemes_FOUND)
    return()
endif()

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)

kate_add_plugin(externaltoolsplugin)
target_compile_definitions(externaltoolsplugin PRIVATE TRANSLATION_DOMAIN="kateexternaltoolsplugin")
target_link_libraries(externaltoolsplugin PRIVATE
    KF6::CoreAddons
    KF6::IconThemes
    KF6::IconWidgets
    KF6::TextEditor
    KF6::I18n
    kateprivate
)

ki18n_wrap_ui(externaltoolsplugin configwidget.ui
    tooldialog.ui
    toolview.ui
)

target_sources(
  externaltoolsplugin
  PRIVATE
    externaltoolsplugin.cpp
    kateexternaltoolsview.cpp
    katetoolrunner.cpp
    kateexternaltool.cpp
    kateexternaltoolscommand.cpp
    kateexternaltoolsconfigwidget.cpp
    plugin.qrc
)

if (ENABLE_PCH)
    target_precompile_headers(externaltoolsplugin REUSE_FROM katepch)
endif()

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
