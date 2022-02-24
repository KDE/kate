kate_add_plugin(lspclientplugin)
target_compile_definitions(lspclientplugin PRIVATE TRANSLATION_DOMAIN="lspclient")

target_link_libraries(
  lspclientplugin
  PRIVATE
    KF5::I18n
    KF5::TextEditor
)

include(ECMQtDeclareLoggingCategory)
ecm_qt_declare_logging_category(
  DEBUG_SOURCES
  HEADER lspclient_debug.h
  IDENTIFIER LSPCLIENT
  CATEGORY_NAME "katelspclientplugin"
)
target_sources(lspclientplugin PRIVATE ${DEBUG_SOURCES})

ki18n_wrap_ui(lspclientplugin lspconfigwidget.ui)
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
    lspclientutils.cpp
    lspsemantichighlighting.cpp
    semantic_tokens_legend.cpp
    lsptooltip.cpp
    gotosymboldialog.cpp

    ${CMAKE_SOURCE_DIR}/shared/quickdialog.cpp # hud dialog

    plugin.qrc
)

target_include_directories(
    lspclientplugin
    PUBLIC
    ${CMAKE_SOURCE_DIR}/shared
)


if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
