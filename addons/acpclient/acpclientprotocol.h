/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QVariant>

#include <optional>
#include <vector>

namespace ACP
{

// ACP Protocol Version - should be integer 1 for v1
const int PROTOCOL_VERSION_INT = 1;
const QString PROTOCOL_VERSION = QStringLiteral("1");

// Message types
const QString METHOD_INITIALIZE = QStringLiteral("initialize");
const QString METHOD_AUTH_LOGIN = QStringLiteral("auth/login");
const QString METHOD_SESSION_NEW = QStringLiteral("session/new");
const QString METHOD_SESSION_LOAD = QStringLiteral("session/load");
const QString METHOD_SESSION_RESUME = QStringLiteral("session/resume");
const QString METHOD_SESSION_CLOSE = QStringLiteral("session/close");
const QString METHOD_SESSION_LIST = QStringLiteral("session/list");
const QString METHOD_SESSION_DELETE = QStringLiteral("session/delete");
const QString METHOD_SESSION_PROMPT = QStringLiteral("session/prompt");
const QString METHOD_SESSION_CANCEL = QStringLiteral("session/cancel");
const QString METHOD_SESSION_REQUEST_PERMISSION = QStringLiteral("session/request_permission");
const QString METHOD_TOOLS_LIST = QStringLiteral("tools/list");
const QString METHOD_TOOLS_CALL = QStringLiteral("tools/call");
const QString METHOD_PROGRESS = QStringLiteral("progress");
const QString METHOD_NOTIFICATION = QStringLiteral("notification");

// Notification types
const QString NOTIFICATION_SESSION_UPDATE = QStringLiteral("session/update");
const QString NOTIFICATION_PROGRESS = QStringLiteral("$/progress");
const QString NOTIFICATION_CANCELLATION = QStringLiteral("$/cancel_request");

// Session update types (from protocol v1)
const QString SESSION_UPDATE_PLAN = QStringLiteral("plan");
const QString SESSION_UPDATE_AGENT_MESSAGE_CHUNK = QStringLiteral("agent_message_chunk");
const QString SESSION_UPDATE_USER_MESSAGE_CHUNK = QStringLiteral("user_message_chunk");
const QString SESSION_UPDATE_THOUGHT_MESSAGE_CHUNK = QStringLiteral("thought_message_chunk");
const QString SESSION_UPDATE_TOOL_CALL = QStringLiteral("tool_call");
const QString SESSION_UPDATE_TOOL_CALL_UPDATE = QStringLiteral("tool_call_update");
const QString SESSION_UPDATE_USAGE_UPDATE = QStringLiteral("usage_update");
const QString SESSION_UPDATE_MODE = QStringLiteral("mode");
const QString SESSION_UPDATE_AVAILABLE_COMMANDS = QStringLiteral("available_commands");

// Permission option kinds
const QString PERMISSION_KIND_ALLOW_ONCE = QStringLiteral("allow_once");
const QString PERMISSION_KIND_ALLOW_ALWAYS = QStringLiteral("allow_always");
const QString PERMISSION_KIND_REJECT_ONCE = QStringLiteral("reject_once");
const QString PERMISSION_KIND_REJECT_ALWAYS = QStringLiteral("reject_always");

// Permission outcome kinds
const QString PERMISSION_OUTCOME_SELECTED = QStringLiteral("selected");
const QString PERMISSION_OUTCOME_CANCELLED = QStringLiteral("cancelled");

// JSON-RPC fields
const QString JSONRPC_VERSION_KEY = QStringLiteral("jsonrpc");
const QString JSONRPC_VERSION_VALUE = QStringLiteral("2.0");
const QString JSONRPC_ID = QStringLiteral("id");
const QString JSONRPC_METHOD = QStringLiteral("method");
const QString JSONRPC_PARAMS = QStringLiteral("params");
const QString JSONRPC_RESULT = QStringLiteral("result");
const QString JSONRPC_ERROR = QStringLiteral("error");

// Client capabilities
struct ClientCapabilities {
    struct FileSystem {
        bool readTextFile = false;
        bool writeTextFile = false;
    } fs;
    bool terminal = false;
    struct BooleanConfigOption {
        bool supported = false;
    } sessionConfigOptionsBoolean;
};

// Agent capabilities
struct AgentCapabilities {
    bool loadSession = false;
    bool supportsSessions = false;
    bool supportsTools = false;
    bool supportsProgress = false;
    bool supportsAuthentication = false;
    struct PromptCapabilities {
        bool image = false;
        bool audio = false;
        bool embeddedContext = false;
    } promptCapabilities;
    struct MCPCapabilities {
        bool http = false;
        bool sse = false;
    } mcpCapabilities;
    struct AuthCapabilities {
        bool logout = false;
    } auth;
    struct SessionCapabilities {
        bool resume = false;
        bool close = false;
        bool deleteSession = false;
        bool additionalDirectories = false;
    } sessionCapabilities;
    QStringList supportedProtocolVersions;
    QJsonObject customCapabilities;
};

// Initialize request parameters
struct InitializeParams {
    ClientCapabilities clientCapabilities;
};

// Initialize response
struct InitializeResult {
    QString agentName;
    QString agentVersion;
    AgentCapabilities capabilities;
    QString protocolVersion;
    QJsonObject metadata;
};

// Authentication methods
struct AuthLoginParams {
    QString providerId;
    QJsonObject providerData;
};

// Session types
struct SessionParams {
    QString sessionId;
    QJsonObject metadata;
};

// Session new parameters
struct SessionNewParams {
    QJsonObject metadata;
    QString cwd;
    QJsonArray mcpServers;
    QJsonArray additionalDirectories;
};

// Session resume parameters
struct SessionResumeParams {
    QString sessionId;
    QString cwd;
    QJsonArray mcpServers;
    QJsonArray additionalDirectories;
};

// Session load parameters
struct SessionLoadParams {
    QString sessionId;
    QString cwd;
    QJsonArray mcpServers;
    QJsonArray additionalDirectories;
};

// Session close parameters
struct SessionCloseParams {
    QString sessionId;
};

// Session delete parameters
struct SessionDeleteParams {
    QString sessionId;
};

// Content types
const QString CONTENT_TYPE_TEXT = QStringLiteral("text");
const QString CONTENT_TYPE_RESOURCE = QStringLiteral("resource");
const QString CONTENT_TYPE_IMAGE = QStringLiteral("image");
const QString CONTENT_TYPE_AUDIO = QStringLiteral("audio");
const QString CONTENT_TYPE_RESOURCE_LINK = QStringLiteral("resourceLink");

// Resource types
const QString RESOURCE_TYPE_URI = QStringLiteral("uri");

// Content block structure
struct ContentBlock {
    QString type;
    QString text; // for text type
    QJsonObject resource; // for resource type
    QString mimeType; // optional
};

// Session prompt parameters
struct SessionPromptParams {
    QString sessionId;
    QList<ContentBlock> prompt;
    QJsonObject metadata;
};

// Session update notification
struct SessionUpdateNotification {
    QString sessionId;
    QString status; // "idle", "working", "completed", "error"
    QString message;
    QJsonObject metadata;
    QString stopReason; // For idle status with completion
};

// Tool definition
struct Tool {
    QString identifier;
    QString description;
    QJsonObject inputSchema;
    QJsonObject metadata;
};

// Tool call
struct ToolCall {
    QString toolIdentifier;
    QJsonObject arguments;
    QString callId;
};

// Progress notification
struct ProgressNotification {
    QString token;
    QJsonObject value;
};

// Cancel request notification
struct CancelRequestNotification {
    QString id; // Request ID to cancel
};

// Error codes
enum class ErrorCode {
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    ServerError = -32000,
    UnknownError = -1
};

// Error object
struct ACPError {
    ErrorCode code;
    QString message;
    QJsonObject data;
};

// Message types for internal processing
struct ACPMessage {
    qint64 id = 0;
    QString method;
    QJsonObject params;
    QJsonObject result;
    ACPError error;
    bool isNotification = false;
    bool isResponse = false;
};

// Convert between JSON and ACP structures
class ACPProtocol
{
public:
    static QJsonDocument createInitializeRequest(const InitializeParams &params, qint64 requestId);
    static QJsonDocument createAuthLoginRequest(const AuthLoginParams &params, qint64 requestId);
    static QJsonDocument createSessionNewRequest(const SessionNewParams &params, qint64 requestId);
    static QJsonDocument createSessionLoadRequest(const SessionLoadParams &params, qint64 requestId);
    static QJsonDocument createSessionResumeRequest(const SessionResumeParams &params, qint64 requestId);
    static QJsonDocument createSessionCloseRequest(const SessionCloseParams &params, qint64 requestId);
    static QJsonDocument createSessionListRequest(qint64 requestId);
    static QJsonDocument createSessionDeleteRequest(const SessionDeleteParams &params, qint64 requestId);
    static QJsonDocument createSessionPromptRequest(const SessionPromptParams &params, qint64 requestId);
    static QJsonDocument createSessionCancelRequest(const QString &sessionId, qint64 requestId);
    static QJsonDocument createToolsListRequest(qint64 requestId);
    static QJsonDocument createToolsCallRequest(const QString &toolId, const QJsonObject &arguments, qint64 requestId);

    static QJsonDocument createSessionUpdateNotification(const SessionUpdateNotification &notification);
    static QJsonDocument createProgressNotification(const ProgressNotification &notification);
    static QJsonDocument createCancelRequestNotification(const CancelRequestNotification &notification);

    // Permission request methods
    static QJsonDocument createPermissionResponse(qint64 requestId, const QString &optionId);
    static QJsonDocument createPermissionResponseCancelled(qint64 requestId);

    static bool parseMessage(const QJsonDocument &doc, ACPMessage &message);
    static bool parseInitializeResponse(const QJsonDocument &doc, InitializeResult &result);
    static bool parseSessionUpdate(const QJsonDocument &doc, SessionUpdateNotification &update);
    static bool parseProgressNotification(const QJsonDocument &doc, ProgressNotification &progress);
    static bool parseErrorResponse(const QJsonDocument &doc, ACPError &error);

    static qint64 generateRequestId();
    static QString getProtocolVersion();
};

} // namespace ACP
