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

// ACP Protocol Version
const QString PROTOCOL_VERSION = QStringLiteral("2.0");

// Message types
const QString METHOD_INITIALIZE = QStringLiteral("initialize");
const QString METHOD_AUTH_LOGIN = QStringLiteral("auth/login");
const QString METHOD_SESSION_NEW = QStringLiteral("session/new");
const QString METHOD_SESSION_RESUME = QStringLiteral("session/resume");
const QString METHOD_SESSION_LIST = QStringLiteral("session/list");
const QString METHOD_SESSION_DELETE = QStringLiteral("session/delete");
const QString METHOD_SESSION_PROMPT = QStringLiteral("session/prompt");
const QString METHOD_SESSION_CANCEL = QStringLiteral("session/cancel");
const QString METHOD_TOOLS_LIST = QStringLiteral("tools/list");
const QString METHOD_TOOLS_CALL = QStringLiteral("tools/call");
const QString METHOD_PROGRESS = QStringLiteral("progress");
const QString METHOD_NOTIFICATION = QStringLiteral("notification");

// Notification types
const QString NOTIFICATION_SESSION_UPDATE = QStringLiteral("session/update");
const QString NOTIFICATION_PROGRESS = QStringLiteral("$/progress");
const QString NOTIFICATION_CANCELLATION = QStringLiteral("$/cancel_request");

// JSON-RPC fields
const QString JSONRPC_VERSION = QStringLiteral("2.0");
const QString JSONRPC_ID = QStringLiteral("id");
const QString JSONRPC_METHOD = QStringLiteral("method");
const QString JSONRPC_PARAMS = QStringLiteral("params");
const QString JSONRPC_RESULT = QStringLiteral("result");
const QString JSONRPC_ERROR = QStringLiteral("error");

// Agent capabilities
struct AgentCapabilities {
    bool supportsSessions = false;
    bool supportsTools = false;
    bool supportsProgress = false;
    bool supportsAuthentication = false;
    QStringList supportedProtocolVersions;
    QJsonObject customCapabilities;
};

// Client capabilities
struct ClientCapabilities {
    bool supportsSessions = true;
    bool supportsTools = true;
    bool supportsProgress = true;
    bool supportsAuthentication = true;
    QStringList supportedProtocolVersions = {PROTOCOL_VERSION};
    QJsonObject customCapabilities;
};

// Initialize request parameters
struct InitializeParams {
    QString clientName;
    QString clientVersion;
    ClientCapabilities capabilities;
    QJsonObject metadata;
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
};

// Session resume parameters
struct SessionResumeParams {
    QString sessionId;
};

// Session prompt parameters
struct SessionPromptParams {
    QString sessionId;
    QString message;
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
    QString id;
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
    static QJsonDocument createInitializeRequest(const InitializeParams &params, const QString &requestId);
    static QJsonDocument createAuthLoginRequest(const AuthLoginParams &params, const QString &requestId);
    static QJsonDocument createSessionNewRequest(const SessionNewParams &params, const QString &requestId);
    static QJsonDocument createSessionResumeRequest(const SessionResumeParams &params, const QString &requestId);
    static QJsonDocument createSessionListRequest(const QString &requestId);
    static QJsonDocument createSessionDeleteRequest(const QString &sessionId, const QString &requestId);
    static QJsonDocument createSessionPromptRequest(const SessionPromptParams &params, const QString &requestId);
    static QJsonDocument createSessionCancelRequest(const QString &sessionId, const QString &requestId);
    static QJsonDocument createToolsListRequest(const QString &requestId);
    static QJsonDocument createToolsCallRequest(const QString &toolId, const QJsonObject &arguments, const QString &requestId);

    static QJsonDocument createSessionUpdateNotification(const SessionUpdateNotification &notification);
    static QJsonDocument createProgressNotification(const ProgressNotification &notification);
    static QJsonDocument createCancelRequestNotification(const CancelRequestNotification &notification);

    static bool parseMessage(const QJsonDocument &doc, ACPMessage &message);
    static bool parseInitializeResponse(const QJsonDocument &doc, InitializeResult &result);
    static bool parseSessionUpdate(const QJsonDocument &doc, SessionUpdateNotification &update);
    static bool parseProgressNotification(const QJsonDocument &doc, ProgressNotification &progress);
    static bool parseErrorResponse(const QJsonDocument &doc, ACPError &error);

    static QString generateRequestId();
    static QString getProtocolVersion();
};

} // namespace ACP