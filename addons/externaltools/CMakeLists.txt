# find_package(KF5TextEditor QUIET REQUIRED)
if(${KF5TextEditor_VERSION} VERSION_LESS 5.63.0)
    return()
endif()

find_package(KF5I18n QUIET)
if(NOT KF5I18n_FOUND)
    return()
endif()

find_package(KF5IconThemes QUIET)
if(NOT KF5IconThemes_FOUND)
    return()
endif()

find_package(Qt5Test QUIET REQUIRED)

add_library(externaltoolsplugin MODULE "")
target_compile_definitions(externaltoolsplugin PRIVATE TRANSLATION_DOMAIN="kateexternaltoolsplugin")
target_link_libraries(externaltoolsplugin PRIVATE
    KF5::CoreAddons
    KF5::IconThemes
    KF5::TextEditor
    KF5::I18n
)

ki18n_wrap_ui(UI_SOURCES configwidget.ui
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
    ${UI_SOURCES}
)

kcoreaddons_desktop_to_json (externaltoolsplugin externaltoolsplugin.desktop)
install(TARGETS externaltoolsplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
