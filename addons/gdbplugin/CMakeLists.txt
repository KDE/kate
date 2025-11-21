# Hasn't been ported to Windows yet.

kate_add_plugin(kategdbplugin)
target_compile_definitions(kategdbplugin PRIVATE TRANSLATION_DOMAIN="kategdbplugin")
target_link_libraries(kategdbplugin PRIVATE kateprivate KF6::I18n KF6::TextEditor KF6::KIOGui)

include(ECMQtDeclareLoggingCategory)
ecm_qt_declare_logging_category(
    DEBUG_SOURCES
    HEADER dapclient_debug.h
    IDENTIFIER DAPCLIENT
    CATEGORY_NAME "kategdbplugin"
)

ki18n_wrap_ui(kategdbplugin debugconfig.ui)

set(DAP_SOURCES
    dap/bus.cpp
    dap/socketbus.cpp
    dap/processbus.cpp
    dap/client.cpp
    dap/entities.cpp
    dap/messages.h
    dap/settings.cpp
    dap/socketprocessbus.cpp
    dap/bus_selector.cpp
)

target_sources(
  kategdbplugin
  PRIVATE
    plugin_kategdb.cpp
    backendinterface.cpp
    configview.cpp
    ioview.cpp
    localsview.cpp
    debugconfigpage.cpp
    stackframe_model.cpp
    stackview.cpp

    ${DAP_SOURCES}
    ${DEBUG_SOURCES}
    json_placeholders.cpp
    dapbackend.cpp
    backend.cpp
    launch_json_reader.cpp

    plugin.qrc 
)

if (ENABLE_PCH)
    target_precompile_headers(kategdbplugin REUSE_FROM katepch)
endif()
