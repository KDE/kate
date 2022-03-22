find_package(KF5I18n ${KF5_DEP_VERSION} QUIET)
if(NOT KF5I18n_FOUND)
    return()
endif()

find_package(KF5IconThemes ${KF5_DEP_VERSION} QUIET)
if(NOT KF5IconThemes_FOUND)
    return()
endif()

find_package(Qt${QT_MAJOR_VERSION}Test ${QT_MIN_VERSION} QUIET REQUIRED)

kate_add_plugin(externaltoolsplugin)
target_compile_definitions(externaltoolsplugin PRIVATE TRANSLATION_DOMAIN="kateexternaltoolsplugin")
target_link_libraries(externaltoolsplugin PRIVATE
    KF5::CoreAddons
    KF5::IconThemes
    KF5::TextEditor
    KF5::I18n
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

target_include_directories(
    externaltoolsplugin
    PUBLIC
    ${CMAKE_SOURCE_DIR}/shared
)

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
