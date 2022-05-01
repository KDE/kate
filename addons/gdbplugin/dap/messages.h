/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef DAP_MESSAGES_H
#define DAP_MESSAGES_H

#include <QString>

#define DAP_CONTENT_LENGTH "Content-Length"
#define DAP_SEP "\r\n"

namespace dap
{
constexpr int DAP_SEP_SIZE = 2;
static const QString DAP_TPL_HEADER_FIELD = QStringLiteral("%1: %2" DAP_SEP);

static const QString DAP_SEQ = QStringLiteral("seq");
static const QString DAP_TYPE = QStringLiteral("type");
static const QString DAP_COMMAND = QStringLiteral("command");
static const QString DAP_ARGUMENTS = QStringLiteral("arguments");
static const QString DAP_BODY = QStringLiteral("body");

// capabilities
static const QString DAP_ADAPTER_ID = QStringLiteral("adapterID");
static const QString DAP_LINES_START_AT1 = QStringLiteral("linesStartAt1");
static const QString DAP_COLUMNS_START_AT2 = QStringLiteral("columnsStartAt1");
static const QString DAP_PATH_FORMAT = QStringLiteral("pathFormat");
static const QString DAP_SUPPORTS_VARIABLE_TYPE = QStringLiteral("supportsVariableType");
static const QString DAP_SUPPORTS_VARIABLE_PAGING = QStringLiteral("supportsVariablePaging");
static const QString DAP_SUPPORTS_RUN_IN_TERMINAL_REQUEST = QStringLiteral("supportsRunInTerminalRequest");
static const QString DAP_SUPPORTS_MEMORY_REFERENCES = QStringLiteral("supportsMemoryReferences");
static const QString DAP_SUPPORTS_PROGRESS_REPORTING = QStringLiteral("supportsProgressReporting");
static const QString DAP_SUPPORTS_INVALIDATED_EVENT = QStringLiteral("supportsInvalidatedEvent");
static const QString DAP_SUPPORTS_MEMORY_EVENT = QStringLiteral("supportsMemoryEvent");

// pathFormat values
static const QString DAP_URI = QStringLiteral("uri");
static const QString DAP_PATH = QStringLiteral("path");

// type values
static const QString DAP_REQUEST = QStringLiteral("request");
static const QString DAP_EVENT = QStringLiteral("event");
static const QString DAP_RESPONSE = QStringLiteral("response");

// command values
static const QString DAP_INITIALIZE = QStringLiteral("initialize");
static const QString DAP_LAUNCH = QStringLiteral("launch");
static const QString DAP_ATTACH = QStringLiteral("attach");
static const QString DAP_MODULES = QStringLiteral("modules");
static const QString DAP_VARIABLES = QStringLiteral("variables");
static const QString DAP_SCOPES = QStringLiteral("scopes");
static const QString DAP_THREADS = QStringLiteral("threads");

// event values
static const QString DAP_OUTPUT = QStringLiteral("output");

// fields
static const QString DAP_NAME = QStringLiteral("name");
static const QString DAP_SYSTEM_PROCESS_ID = QStringLiteral("systemProcessId");
static const QString DAP_IS_LOCAL_PROCESS = QStringLiteral("isLocalProcess");
static const QString DAP_POINTER_SIZE = QStringLiteral("pointerSize");
static const QString DAP_START_METHOD = QStringLiteral("startMethod");
static const QString DAP_DATA = QStringLiteral("data");
static const QString DAP_VARIABLES_REFERENCE = QStringLiteral("variablesReference");
static const QString DAP_SOURCE = QStringLiteral("source");
static const QString DAP_GROUP = QStringLiteral("group");
static const QString DAP_LINE = QStringLiteral("line");
static const QString DAP_COLUMN = QStringLiteral("column");
static const QString DAP_PRESENTATION_HINT = QStringLiteral("presentationHint");
static const QString DAP_SOURCES = QStringLiteral("sources");
static const QString DAP_CHECKSUMS = QStringLiteral("checksums");
static const QString DAP_CATEGORY = QStringLiteral("category");
static const QString DAP_THREAD_ID = QStringLiteral("threadId");
static const QString DAP_ID = QStringLiteral("id");
static const QString DAP_MODULE_ID = QStringLiteral("moduleId");
static const QString DAP_REASON = QStringLiteral("reason");
static const QString DAP_FRAME_ID = QStringLiteral("frameId");
static const QString DAP_FILTER = QStringLiteral("filter");
static const QString DAP_START = QStringLiteral("start");
static const QString DAP_COUNT = QStringLiteral("count");
static const QString DAP_SINGLE_THREAD = QStringLiteral("singleThread");
static const QString DAP_ALL_THREADS_CONTINUED = QStringLiteral("allThreadsContinued");
static const QString DAP_SOURCE_REFERENCE = QStringLiteral("sourceReference");
static const QString DAP_BREAKPOINTS = QStringLiteral("breakpoints");
static const QString DAP_ADAPTER_DATA = QStringLiteral("adapterData");
static const QString DAP_CONDITION = QStringLiteral("condition");
static const QString DAP_HIT_CONDITION = QStringLiteral("hitCondition");
static const QString DAP_LOG_MESSAGE = QStringLiteral("logMessage");
static const QString DAP_LINES = QStringLiteral("lines");
static const QString DAP_ORIGIN = QStringLiteral("origin");
static const QString DAP_CHECKSUM = QStringLiteral("checksum");
static const QString DAP_ALGORITHM = QStringLiteral("algorithm");
static const QString DAP_BREAKPOINT = QStringLiteral("breakpoint");
static const QString DAP_EXPRESSION = QStringLiteral("expression");
static const QString DAP_CONTEXT = QStringLiteral("context");
static const QString DAP_RESULT = QStringLiteral("result");
static const QString DAP_TARGET_ID = QStringLiteral("targetId");
static const QString DAP_END_LINE = QStringLiteral("endLine");
static const QString DAP_END_COLUMN = QStringLiteral("endColumn");

}

#endif // DAP_MESSAGES_H
