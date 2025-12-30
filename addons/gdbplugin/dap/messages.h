/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QString>

#define DAP_CONTENT_LENGTH "Content-Length"
#define DAP_SEP "\r\n"

namespace dap
{
constexpr inline int DAP_SEP_SIZE = 2;
static constexpr inline QLatin1String DAP_TPL_HEADER_FIELD("%1: %2" DAP_SEP);

static constexpr inline QLatin1String DAP_SEQ("seq");
static constexpr inline QLatin1String DAP_TYPE("type");
static constexpr inline QLatin1String DAP_COMMAND("command");
static constexpr inline QLatin1String DAP_ARGUMENTS("arguments");
static constexpr inline QLatin1String DAP_BODY("body");

// capabilities
static constexpr inline QLatin1String DAP_ADAPTER_ID("adapterID");
static constexpr inline QLatin1String DAP_LINES_START_AT1("linesStartAt1");
static constexpr inline QLatin1String DAP_COLUMNS_START_AT2("columnsStartAt1");
static constexpr inline QLatin1String DAP_PATH_FORMAT("pathFormat");
static constexpr inline QLatin1String DAP_SUPPORTS_VARIABLE_TYPE("supportsVariableType");
static constexpr inline QLatin1String DAP_SUPPORTS_VARIABLE_PAGING("supportsVariablePaging");
static constexpr inline QLatin1String DAP_SUPPORTS_RUN_IN_TERMINAL_REQUEST("supportsRunInTerminalRequest");
static constexpr inline QLatin1String DAP_SUPPORTS_MEMORY_REFERENCES("supportsMemoryReferences");
static constexpr inline QLatin1String DAP_SUPPORTS_PROGRESS_REPORTING("supportsProgressReporting");
static constexpr inline QLatin1String DAP_SUPPORTS_INVALIDATED_EVENT("supportsInvalidatedEvent");
static constexpr inline QLatin1String DAP_SUPPORTS_MEMORY_EVENT("supportsMemoryEvent");

// pathFormat values
static constexpr inline QLatin1String DAP_URI("uri");
static constexpr inline QLatin1String DAP_PATH("path");

// type values
static constexpr inline QLatin1String DAP_REQUEST("request");
static constexpr inline QLatin1String DAP_EVENT("event");
static constexpr inline QLatin1String DAP_RESPONSE("response");

// command values
static constexpr inline QLatin1String DAP_INITIALIZE("initialize");
static constexpr inline QLatin1String DAP_LAUNCH("launch");
static constexpr inline QLatin1String DAP_ATTACH("attach");
static constexpr inline QLatin1String DAP_MODULES("modules");
static constexpr inline QLatin1String DAP_VARIABLES("variables");
static constexpr inline QLatin1String DAP_SCOPES("scopes");
static constexpr inline QLatin1String DAP_THREADS("threads");
static constexpr inline QLatin1String DAP_RUN_IN_TERMINAL("runInTerminal");

// event values
static constexpr inline QLatin1String DAP_OUTPUT("output");

// fields
static constexpr inline QLatin1String DAP_NAME("name");
static constexpr inline QLatin1String DAP_SYSTEM_PROCESS_ID("systemProcessId");
static constexpr inline QLatin1String DAP_IS_LOCAL_PROCESS("isLocalProcess");
static constexpr inline QLatin1String DAP_POINTER_SIZE("pointerSize");
static constexpr inline QLatin1String DAP_START_METHOD("startMethod");
static constexpr inline QLatin1String DAP_DATA("data");
static constexpr inline QLatin1String DAP_VARIABLES_REFERENCE("variablesReference");
static constexpr inline QLatin1String DAP_SOURCE("source");
static constexpr inline QLatin1String DAP_GROUP("group");
static constexpr inline QLatin1String DAP_LINE("line");
static constexpr inline QLatin1String DAP_COLUMN("column");
static constexpr inline QLatin1String DAP_PRESENTATION_HINT("presentationHint");
static constexpr inline QLatin1String DAP_SOURCES("sources");
static constexpr inline QLatin1String DAP_CHECKSUMS("checksums");
static constexpr inline QLatin1String DAP_CATEGORY("category");
static constexpr inline QLatin1String DAP_THREAD_ID("threadId");
static constexpr inline QLatin1String DAP_ID("id");
static constexpr inline QLatin1String DAP_MODULE_ID("moduleId");
static constexpr inline QLatin1String DAP_REASON("reason");
static constexpr inline QLatin1String DAP_FRAME_ID("frameId");
static constexpr inline QLatin1String DAP_FILTER("filter");
static constexpr inline QLatin1String DAP_START("start");
static constexpr inline QLatin1String DAP_COUNT("count");
static constexpr inline QLatin1String DAP_SINGLE_THREAD("singleThread");
static constexpr inline QLatin1String DAP_ALL_THREADS_CONTINUED("allThreadsContinued");
static constexpr inline QLatin1String DAP_SOURCE_REFERENCE("sourceReference");
static constexpr inline QLatin1String DAP_BREAKPOINTS("breakpoints");
static constexpr inline QLatin1String DAP_ADAPTER_DATA("adapterData");
static constexpr inline QLatin1String DAP_CONDITION("condition");
static constexpr inline QLatin1String DAP_HIT_CONDITION("hitCondition");
static constexpr inline QLatin1String DAP_LOG_MESSAGE("logMessage");
static constexpr inline QLatin1String DAP_LINES("lines");
static constexpr inline QLatin1String DAP_ORIGIN("origin");
static constexpr inline QLatin1String DAP_CHECKSUM("checksum");
static constexpr inline QLatin1String DAP_ALGORITHM("algorithm");
static constexpr inline QLatin1String DAP_BREAKPOINT("breakpoint");
static constexpr inline QLatin1String DAP_EXPRESSION("expression");
static constexpr inline QLatin1String DAP_CONTEXT("context");
static constexpr inline QLatin1String DAP_RESULT("result");
static constexpr inline QLatin1String DAP_TARGET_ID("targetId");
static constexpr inline QLatin1String DAP_END_LINE("endLine");
static constexpr inline QLatin1String DAP_END_COLUMN("endColumn");
static constexpr inline QLatin1String DAP_SUCCESS("success");
static constexpr inline QLatin1String DAP_REQUEST_SEQ("request_seq");
}
