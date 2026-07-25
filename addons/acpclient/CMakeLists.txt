kate_add_plugin(acpclientplugin)
target_compile_definitions(acpclientplugin PRIVATE TRANSLATION_DOMAIN="acpclient")

target_link_libraries(
  acpclientplugin
  PRIVATE
    KF6::I18n
    KF6::TextEditor
)

target_sources(
  acpclientplugin
  PRIVATE
    acpclientplugin.cpp
    acpclientpluginview.cpp
    acpclientconfigpage.cpp
    acpserverdialog.cpp
    acpclientserver.cpp
    acpclientservermanager.cpp
    acpclientprotocol.cpp
    acpclientchatwidget.cpp
    acpchatmessagewidget.cpp
    acpclient_debug.h
    acpsessionlistwidget.cpp
    acpsessionlistwidget.h

    plugin.qrc
)

include(ECMQtDeclareLoggingCategory)
ecm_qt_declare_logging_category(
  DEBUG_SOURCES
  HEADER acpclient_debug.h
  IDENTIFIER ACPCLIENT
  CATEGORY_NAME "kateacpclientplugin"
)
target_sources(acpclientplugin PRIVATE ${DEBUG_SOURCES})

ki18n_wrap_ui(acpclientplugin
    acpconfigwidget.ui
    acpserverdialog.ui
    acpclientchat.ui
)

qt_add_resources(acpclientplugin plugin.qrc
    PREFIX "/kxmlgui5/kateacpclient"
    FILES ui.rc
)

if (ENABLE_PCH)
    target_precompile_headers(acpclientplugin REUSE_FROM katepch)
endif()

if(BUILD_TESTING)
  # add_subdirectory(tests) - Add tests directory if needed later
endif()