find_package(KF5ItemModels ${KF5_DEP_VERSION} QUIET)
set_package_properties(KF5ItemModels PROPERTIES PURPOSE "Required to build the lspclient addon")

find_package(KF5SyntaxHighlighting ${KF5_DEP_VERSION} QUIET)
set_package_properties(KF5SyntaxHighlighting PROPERTIES PURPOSE "Required to build the lspclient addon")

if(NOT KF5ItemModels_FOUND OR NOT KF5SyntaxHighlighting_FOUND)
  return()
endif()

add_library(lspclientplugin MODULE "")
target_compile_definitions(lspclientplugin PRIVATE TRANSLATION_DOMAIN="lspclient")

ki18n_wrap_ui(UI_SOURCES lspconfigwidget.ui)

target_link_libraries(
  lspclientplugin
  PRIVATE
    KF5::ItemModels
    KF5::TextEditor
    KF5::SyntaxHighlighting
)

include(ECMQtDeclareLoggingCategory)
ecm_qt_declare_logging_category(
  DEBUG_SOURCES
  HEADER lspclient_debug.h
  IDENTIFIER LSPCLIENT
  CATEGORY_NAME "katelspclientplugin"
)
target_sources(lspclientplugin PRIVATE ${DEBUG_SOURCES})

target_sources(
  lspclientplugin
  PRIVATE
    lspclientcompletion.cpp
    lspclientconfigpage.cpp
    lspclienthover.cpp
    lspclientplugin.cpp
    lspclientpluginview.cpp
    lspclientserver.cpp
    lspclientservermanager.cpp
    lspclientsymbolview.cpp
    plugin.qrc
    ${UI_SOURCES}
)

target_include_directories(
    lspclientplugin
    PUBLIC
    ${CMAKE_SOURCE_DIR}/shared
)

kcoreaddons_desktop_to_json(lspclientplugin lspclientplugin.desktop)
install(TARGETS lspclientplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
