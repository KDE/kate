find_package(KF${KF_MAJOR_VERSION}I18n ${KF5_DEP_VERSION} QUIET)
if(NOT KF${KF_MAJOR_VERSION}I18n_FOUND)
    return()
endif()

find_package(KF${KF_MAJOR_VERSION}IconThemes ${KF5_DEP_VERSION} QUIET)
if(NOT KF${KF_MAJOR_VERSION}IconThemes_FOUND)
    return()
endif()

find_package(Qt${QT_MAJOR_VERSION}Test ${QT_MIN_VERSION} QUIET REQUIRED)

kate_add_plugin(externaltoolsplugin)
target_compile_definitions(externaltoolsplugin PRIVATE TRANSLATION_DOMAIN="kateexternaltoolsplugin")
target_link_libraries(externaltoolsplugin PRIVATE
    KF${KF_MAJOR_VERSION}::CoreAddons
    KF${KF_MAJOR_VERSION}::IconThemes
    KF${KF_MAJOR_VERSION}::IconWidgets
    KF${KF_MAJOR_VERSION}::TextEditor
    KF${KF_MAJOR_VERSION}::I18n
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

if (BUILD_PCH)
    target_precompile_headers(externaltoolsplugin REUSE_FROM katepch)
endif()

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
