# SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>
#
# SPDX-License-Identifier: MIT

kate_add_plugin(acpclientplugin)
target_compile_definitions(acpclientplugin PRIVATE TRANSLATION_DOMAIN="acpclient")

target_link_libraries(
    acpclientplugin
    PRIVATE
    kateprivate
)

target_sources(
  acpclientplugin
  PRIVATE
    acpclientplugin.cpp
    acpclientpluginview.cpp
    acpclientconfigpage.cpp
    acpclientserver.cpp
    acpclientservermanager.cpp
    acpclientprotocol.cpp
    acpclientchatwidget.cpp
    acpchatmessagewidget.cpp
    acpsessionlistwidget.cpp
    acpsessionlistwidget.h
    acpserverlistwidget.cpp
    acpserverlistwidget.h

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
    acpclientchat.ui
)

if (ENABLE_PCH)
    target_precompile_headers(acpclientplugin REUSE_FROM katepch)
endif()

if(BUILD_TESTING)
  # add_subdirectory(tests) - Add tests directory if needed later
endif()
