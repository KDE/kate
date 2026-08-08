/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

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

/**
 * @namespace ACP
 * @brief Agent Client Protocol (ACP) implementation for Kate
 *
 * This namespace contains the ACP Protocol v1 implementation as specified at
 * https://agentclientprotocol.com
 *
 * The protocol uses JSON-RPC 2.0 over stdin/stdout with newline-delimited messages.
 * Key concepts:
 * - Request/Response pattern for client-initiated actions
 * - Notification pattern for server-initiated updates
 * - Session management for maintaining conversation state
 * - Tool calls for agent capabilities
 * - Permission requests for user approval of agent actions
 */
namespace ACP
{

// ACP Protocol Version - should be integer 1 for v1
inline int protocolVersionInt()
{
    return 1;
}
inline QString protocolVersion()
{
    return QStringLiteral("1");
}

// ============================================================================
// ACP METHOD CONSTANTS
// Client-to-Server request methods
// ============================================================================

/** @brief Initialize a new ACP connection */
inline QString methodInitialize()
{
    return QStringLiteral("initialize");
}

/** @brief Authenticate with an auth provider */
inline QString methodAuthLogin()
{
    return QStringLiteral("auth/login");
}

/** @brief Create a new conversation session */
inline QString methodSessionNew()
{
    return QStringLiteral("session/new");
}

/** @brief Load a previously saved session */
inline QString methodSessionLoad()
{
    return QStringLiteral("session/load");
}

/** @brief Resume a paused session */
inline QString methodSessionResume()
{
    return QStringLiteral("session/resume");
}

/** @brief Close an active session */
inline QString methodSessionClose()
{
    return QStringLiteral("session/close");
}

/** @brief List all available sessions */
inline QString methodSessionList()
{
    return QStringLiteral("session/list");
}

/** @brief Delete a session */
inline QString methodSessionDelete()
{
    return QStringLiteral("session/delete");
}

/** @brief Send a prompt/message to a session */
inline QString methodSessionPrompt()
{
    return QStringLiteral("session/prompt");
}

/** @brief Cancel the current agent operation */
inline QString methodSessionCancel()
{
    return QStringLiteral("session/cancel");
}

/** @brief Request permission for a tool call (server -> client notification) */
inline QString methodSessionRequestPermission()
{
    return QStringLiteral("session/request_permission");
}

/** @brief List available tools from the agent */
inline QString methodToolsList()
{
    return QStringLiteral("tools/list");
}

/** @brief Call a specific tool */
inline QString methodToolsCall()
{
    return QStringLiteral("tools/call");
}

/** @brief Send progress update (rarely used, prefer $/progress notification) */
inline QString methodProgress()
{
    return QStringLiteral("progress");
}

/** @brief Generic notification method */
inline QString methodNotification()
{
    return QStringLiteral("notification");
}

// ============================================================================
// NOTIFICATION CONSTANTS
// Server-to-Client notifications (no response expected)
// ============================================================================

/** @brief Session state update notification (streaming responses) */
inline QString notificationSessionUpdate()
{
    return QStringLiteral("session/update");
}

/** @brief Progress notification for long-running operations */
inline QString notificationProgress()
{
    return QStringLiteral("$/progress");
}

/** @brief Cancellation request notification */
inline QString notificationCancellation()
{
    return QStringLiteral("$/cancel_request");
}

// ============================================================================
// SESSION UPDATE TYPES
// Used in session/update notifications to identify the update type
// ============================================================================

/** @brief Agent is sending a plan/step update */
inline QString sessionUpdatePlan()
{
    return QStringLiteral("plan");
}

/** @brief Streaming chunk of agent's text response */
inline QString sessionUpdateAgentMessageChunk()
{
    return QStringLiteral("agent_message_chunk");
}

/** @brief Streaming chunk of user message (echo) */
inline QString sessionUpdateUserMessageChunk()
{
    return QStringLiteral("user_message_chunk");
}

/** @brief Streaming chunk of agent's thought process */
inline QString sessionUpdateThoughtMessageChunk()
{
    return QStringLiteral("thought_message_chunk");
}

/** @brief Notification that agent is calling a tool */
inline QString sessionUpdateToolCall()
{
    return QStringLiteral("tool_call");
}

/** @brief Progress update for a running tool call */
inline QString sessionUpdateToolCallUpdate()
{
    return QStringLiteral("tool_call_update");
}

/** @brief Token usage or cost information update */
inline QString sessionUpdateUsageUpdate()
{
    return QStringLiteral("usage_update");
}

/** @brief Agent mode change notification */
inline QString sessionUpdateMode()
{
    return QStringLiteral("mode");
}

/** @brief Available commands/actions update */
inline QString sessionUpdateAvailableCommands()
{
    return QStringLiteral("available_commands");
}

// ============================================================================
// PERMISSION CONSTANTS
// ============================================================================

/** @brief Allow the action once */
inline QString permissionKindAllowOnce()
{
    return QStringLiteral("allow_once");
}

/** @brief Allow the action and remember for future requests */
inline QString permissionKindAllowAlways()
{
    return QStringLiteral("allow_always");
}

/** @brief Reject the action once */
inline QString permissionKindRejectOnce()
{
    return QStringLiteral("reject_once");
}

/** @brief Reject the action and remember for future requests */
inline QString permissionKindRejectAlways()
{
    return QStringLiteral("reject_always");
}

/** @brief User selected a permission option */
inline QString permissionOutcomeSelected()
{
    return QStringLiteral("selected");
}

/** @brief User cancelled the permission request */
inline QString permissionOutcomeCancelled()
{
    return QStringLiteral("cancelled");
}

// ============================================================================
// JSON-RPC FIELD CONSTANTS
// ============================================================================

/** @brief JSON-RPC version field name */
inline QString jsonrpcVersionKey()
{
    return QStringLiteral("jsonrpc");
}

/** @brief JSON-RPC version value (always 2.0) */
inline QString jsonrpcVersionValue()
{
    return QStringLiteral("2.0");
}

/** @brief Request/response ID field name */
inline QString jsonrpcId()
{
    return QStringLiteral("id");
}

/** @brief Method name field */
inline QString jsonrpcMethod()
{
    return QStringLiteral("method");
}

/** @brief Parameters object field */
inline QString jsonrpcParams()
{
    return QStringLiteral("params");
}

/** @brief Result object field (in responses) */
inline QString jsonrpcResult()
{
    return QStringLiteral("result");
}

/** @brief Error object field (in error responses) */
inline QString jsonrpcError()
{
    return QStringLiteral("error");
}

// ============================================================================
// CAPABILITY STRUCTURES
// ============================================================================

/**
 * @brief Client capabilities advertised to the ACP server
 *
 * Defines what the client (Kate) supports for the agent to use.
 */
struct ClientCapabilities {
    /** File system operations the client supports */
    struct FileSystem {
        bool readTextFile = false; ///< Client can read text files
        bool writeTextFile = false; ///< Client can write text files
    } fs;
    bool terminal = false; ///< Client has terminal access
    struct BooleanConfigOption {
        bool supported = false; ///< Client supports boolean session config options
    } sessionConfigOptionsBoolean;
};

/**
 * @brief Agent capabilities received from the ACP server
 *
 * Defines what the agent supports. Populated from the initialize response.
 */
struct AgentCapabilities {
    bool loadSession = false; ///< Agent can load existing sessions
    bool supportsSessions = false; ///< Agent supports session management
    bool supportsTools = false; ///< Agent has tools available
    bool supportsProgress = false; ///< Agent can send progress updates
    bool supportsAuthentication = false; ///< Agent requires authentication

    /** Prompt content types the agent accepts */
    struct PromptCapabilities {
        bool image = false; ///< Accepts image content in prompts
        bool audio = false; ///< Accepts audio content in prompts
        bool embeddedContext = false; ///< Accepts embedded context in prompts
    } promptCapabilities;

    /** Model Context Protocol (MCP) support */
    struct MCPCapabilities {
        bool http = false; ///< Supports MCP over HTTP
        bool sse = false; ///< Supports MCP over Server-Sent Events
    } mcpCapabilities;

    /** Authentication capabilities */
    struct AuthCapabilities {
        bool logout = false; ///< Supports logout functionality
    } auth;

    /** Session management capabilities */
    struct SessionCapabilities {
        bool resume = false; ///< Can resume paused sessions
        bool close = false; ///< Can close sessions
        bool deleteSession = false; ///< Can delete sessions
        bool additionalDirectories = false; ///< Supports additional directories in session
    } sessionCapabilities;

    QStringList supportedProtocolVersions; ///< List of supported ACP protocol versions
    QJsonObject customCapabilities; ///< Agent-specific custom capabilities
};

// ============================================================================
// REQUEST/RESPONSE PARAMETER STRUCTURES
// ============================================================================

/** @brief Client info for initialize request */
struct ClientInfo {
    QString name; ///< Client name (e.g., "kate")
    QString title; ///< Human-readable client title
    QString version; ///< Client version
};

/** @brief Parameters for initialize request */
struct InitializeParams {
    int protocolVersion; ///< Protocol version (should be 1 for v1)
    ClientCapabilities clientCapabilities; ///< Capabilities this client supports
    ClientInfo clientInfo; ///< Information about the client
};

/** @brief Agent info from initialize response */
struct AgentInfo {
    QString name; ///< Name of the agent (e.g., "Mistral Vibe")
    QString title; ///< Human-readable agent title
    QString version; ///< Version of the agent
};

/** @brief Result from initialize request */
struct InitializeResult {
    QString protocolVersion; ///< Negotiated protocol version
    AgentCapabilities capabilities; ///< What the agent supports
    AgentInfo agentInfo; ///< Information about the agent
    QJsonArray authMethods; ///< Available authentication methods
};

/** @brief Parameters for auth/login request */
struct AuthLoginParams {
    QString providerId; ///< Authentication provider ID
    QJsonObject providerData; ///< Provider-specific authentication data
};

/** @brief Base session parameters (used in multiple session methods) */
struct SessionParams {
    QString sessionId; ///< Unique session identifier
};

/** @brief Parameters for session/new request */
struct SessionNewParams {
    QString cwd; ///< Working directory for the session
    QJsonArray mcpServers; ///< MCP servers to connect to
    QJsonArray additionalDirectories; ///< Additional directories for file access
};

/** @brief Parameters for session/resume request */
struct SessionResumeParams {
    QString sessionId; ///< Session to resume
    QString cwd; ///< Working directory
    QJsonArray mcpServers; ///< MCP servers to connect to
    QJsonArray additionalDirectories; ///< Additional directories for file access
};

/** @brief Parameters for session/load request */
struct SessionLoadParams {
    QString sessionId; ///< Session to load
    QString cwd; ///< Working directory
    QJsonArray mcpServers; ///< MCP servers to connect to
    QJsonArray additionalDirectories; ///< Additional directories for file access
};

/** @brief Parameters for session/close request */
struct SessionCloseParams {
    QString sessionId; ///< Session to close
};

/** @brief Parameters for session/delete request */
struct SessionDeleteParams {
    QString sessionId; ///< Session to delete
};

/** @brief Parameters for session/list request */
struct ListSessionsRequest {
    int limit = -1; ///< Maximum number of sessions to return (-1 for no limit)
    int offset = 0; ///< Number of sessions to skip for pagination
};

// ============================================================================
// CONTENT TYPES
// ============================================================================

/** @brief Plain text content */
inline QString contentTypeText()
{
    return QStringLiteral("text");
}

/** @brief Resource content (file, etc.) */
inline QString contentTypeResource()
{
    return QStringLiteral("resource");
}

/** @brief Image content */
inline QString contentTypeImage()
{
    return QStringLiteral("image");
}

/** @brief Audio content */
inline QString contentTypeAudio()
{
    return QStringLiteral("audio");
}

/** @brief Resource link (URI reference) */
inline QString contentTypeResourceLink()
{
    return QStringLiteral("resourceLink");
}

// ============================================================================
// RESOURCE TYPES
// ============================================================================

/** @brief URI resource type */
inline QString resourceTypeUri()
{
    return QStringLiteral("uri");
}

// ============================================================================
// CONTENT STRUCTURES
// ============================================================================

/**
 * @brief A content block in a prompt or message
 *
 * ACP supports rich content including text, resources, images, and audio.
 * Each content block has a type and type-specific data.
 */
struct ContentBlock {
    QString type; ///< Content type (contentTypeText, contentTypeResource, etc.)
    QString text; ///< Text content (for type == contentTypeText())
    QJsonObject resource; ///< Resource object (for type == contentTypeResource())
    QString mimeType; ///< Optional MIME type
};

/** @brief Parameters for session/prompt request */
struct SessionPromptParams {
    QString sessionId; ///< Target session
    QList<ContentBlock> prompt; ///< List of content blocks to send
};

// ============================================================================
// NOTIFICATION STRUCTURES
// ============================================================================

/**
 * @brief Session update notification parameters
 *
 * Sent by server to update client on session state changes.
 * According to ACP spec, params contain sessionId and update object.
 */
struct SessionUpdateNotification {
    QString sessionId; ///< The session being updated
    QJsonObject update; ///< The update object with type-specific data
};

// ============================================================================
// TOOL STRUCTURES
// ============================================================================

/** @brief Tool definition from tools/list response */
struct Tool {
    QString identifier; ///< Unique tool identifier
    QString description; ///< Human-readable tool description
    QJsonObject inputSchema; ///< JSON Schema for tool arguments
};

/** @brief Tool call parameters for tools/call request */
struct ToolCall {
    QString toolIdentifier; ///< Tool to call
    QJsonObject arguments; ///< Arguments for the tool
    QString callId; ///< Unique call identifier
};

// ============================================================================
// PROGRESS AND CANCEL STRUCTURES
// ============================================================================

/** @brief Progress notification parameters */
struct ProgressNotification {
    QString token; ///< Progress token (matches initial request)
    QJsonObject value; ///< Progress value object
};

/** @brief Cancel request notification parameters */
struct CancelRequestNotification {
    QString id; ///< Request ID to cancel
};

// ============================================================================
// ERROR HANDLING
// ============================================================================

/** @brief ACP error codes (JSON-RPC standard codes) */
enum class ErrorCode {
    ParseError = -32700, ///< Invalid JSON
    InvalidRequest = -32600, ///< JSON is valid but not a valid Request
    MethodNotFound = -32601, ///< Method does not exist
    InvalidParams = -32602, ///< Invalid method parameters
    InternalError = -32603, ///< Internal JSON-RPC error
    ServerError = -32000, ///< Server-specific error (start of server error range)
    UnknownError = -1 ///< Unknown/unclassified error
};

/** @brief ACP error object (from error responses) */
struct ACPError {
    ErrorCode code; ///< Error code
    QString message; ///< Human-readable error message
    QJsonObject data; ///< Additional error data
};

// ============================================================================
// INTERNAL MESSAGE STRUCTURE
// ============================================================================

/**
 * @brief Internal representation of a parsed ACP message
 *
 * Used for parsing and routing incoming JSON-RPC messages.
 */
struct ACPMessage {
    qint64 id = 0; ///< Request/response ID (0 for notifications)
    QString method; ///< Method name
    QJsonObject params; ///< Parameters object
    QJsonObject result; ///< Result object (for responses)
    ACPError error; ///< Error object (for error responses)
    bool isNotification = false; ///< True for notifications (no id, has method)
    bool isResponse = false; ///< True for responses (has id, has result or error)
};

// ============================================================================
// ACP PROTOCOL UTILITY CLASS
// ============================================================================

/**
 * @brief Utility class for ACP protocol message creation and parsing
 *
 * Provides static methods to create ACP-compliant JSON-RPC messages and
 * parse incoming messages into structured data.
 *
 * All methods are static - this is a utility class, not meant to be instantiated.
 *
 * @note The ACP protocol uses JSON-RPC 2.0 with the following conventions:
 *       - All messages are valid JSON objects
 *       - Messages are newline-delimited over stdin/stdout
 *       - Requests have: jsonrpc, id, method, params
 *       - Responses have: jsonrpc, id, result (or error)
 *       - Notifications have: jsonrpc, method, params (no id)
 */
class ACPProtocol
{
public:
    // ========================================================================
    // REQUEST CREATION METHODS
    // Create JSON-RPC request documents for client-to-server communication
    // ========================================================================

    /**
     * @brief Create an initialize request
     * @param params Initialization parameters including client capabilities
     * @param requestId Unique request identifier
     * @return JSON-RPC 2.0 initialize request document
     */
    static QJsonDocument createInitializeRequest(const InitializeParams &params, qint64 requestId);

    /**
     * @brief Create an authentication login request
     * @param params Authentication parameters
     * @param requestId Unique request identifier
     * @return JSON-RPC 2.0 auth/login request document
     */
    static QJsonDocument createAuthLoginRequest(const AuthLoginParams &params, qint64 requestId);

    /**
     * @brief Create a session/new request to start a new conversation
     * @param params Session parameters
     * @param requestId Unique request identifier
     * @return JSON-RPC 2.0 session/new request document
     */
    static QJsonDocument createSessionNewRequest(const SessionNewParams &params, qint64 requestId);

    /**
     * @brief Create a session/load request to load a saved session
     * @param params Session load parameters
     * @param requestId Unique request identifier
     * @return JSON-RPC 2.0 session/load request document
     */
    static QJsonDocument createSessionLoadRequest(const SessionLoadParams &params, qint64 requestId);

    /**
     * @brief Create a session/resume request to continue a paused session
     * @param params Session resume parameters
     * @param requestId Unique request identifier
     * @return JSON-RPC 2.0 session/resume request document
     */
    static QJsonDocument createSessionResumeRequest(const SessionResumeParams &params, qint64 requestId);

    /**
     * @brief Create a session/close request to end a session
     * @param params Session close parameters
     * @param requestId Unique request identifier
     * @return JSON-RPC 2.0 session/close request document
     */
    static QJsonDocument createSessionCloseRequest(const SessionCloseParams &params, qint64 requestId);

    /**
     * @brief Create a session/list request to get all sessions
     * @param params List sessions parameters (optional limit/offset)
     * @param requestId Unique request identifier
     * @return JSON-RPC 2.0 session/list request document
     */
    static QJsonDocument createSessionListRequest(const ListSessionsRequest &params, qint64 requestId);

    /**
     * @brief Create a session/delete request to remove a session
     * @param params Session delete parameters
     * @param requestId Unique request identifier
     * @return JSON-RPC 2.0 session/delete request document
     */
    static QJsonDocument createSessionDeleteRequest(const SessionDeleteParams &params, qint64 requestId);

    /**
     * @brief Create a session/prompt request to send a message to a session
     * @param params Prompt parameters including session ID and content
     * @param requestId Unique request identifier
     * @return JSON-RPC 2.0 session/prompt request document
     */
    static QJsonDocument createSessionPromptRequest(const SessionPromptParams &params, qint64 requestId);

    /**
     * @brief Create a session/cancel request to abort current operation
     * @param sessionId Target session
     * @param requestId Unique request identifier
     * @return JSON-RPC 2.0 session/cancel request document
     */
    static QJsonDocument createSessionCancelRequest(const QString &sessionId, qint64 requestId);

    /**
     * @brief Create a tools/list request to get available tools
     * @param requestId Unique request identifier
     * @return JSON-RPC 2.0 tools/list request document
     */
    static QJsonDocument createToolsListRequest(qint64 requestId);

    /**
     * @brief Create a tools/call request to invoke a tool
     * @param toolId Tool identifier
     * @param arguments Tool arguments
     * @param requestId Unique request identifier
     * @return JSON-RPC 2.0 tools/call request document
     */
    static QJsonDocument createToolsCallRequest(const QString &toolId, const QJsonObject &arguments, qint64 requestId);

    // ========================================================================
    // NOTIFICATION CREATION METHODS
    // Create JSON-RPC notifications for client-to-server communication
    // ========================================================================

    /**
     * @brief Create a session/update notification (rarely used by client)
     * @param notification Notification parameters
     * @return JSON-RPC 2.0 session/update notification document
     */
    static QJsonDocument createSessionUpdateNotification(const SessionUpdateNotification &notification);

    /**
     * @brief Create a $/progress notification
     * @param notification Progress parameters
     * @return JSON-RPC 2.0 $/progress notification document
     */
    static QJsonDocument createProgressNotification(const ProgressNotification &notification);

    /**
     * @brief Create a $/cancel_request notification
     * @param notification Cancel request parameters
     * @return JSON-RPC 2.0 $/cancel_request notification document
     */
    static QJsonDocument createCancelRequestNotification(const CancelRequestNotification &notification);

    // ========================================================================
    // PERMISSION RESPONSE METHODS
    // Create responses to permission requests
    // ========================================================================

    /**
     * @brief Create a permission response (user approved or rejected)
     * @param requestId The permission request ID to respond to
     * @param optionId The selected option (PERMISSION_KIND_*)
     * @return JSON-RPC 2.0 response document with permission outcome
     */
    static QJsonDocument createPermissionResponse(qint64 requestId, const QString &optionId);

    /**
     * @brief Create a cancelled permission response
     * @param requestId The permission request ID that was cancelled
     * @return JSON-RPC 2.0 response document with cancelled outcome
     */
    static QJsonDocument createPermissionResponseCancelled(qint64 requestId);

    // ========================================================================
    // PARSING METHODS
    // Parse incoming JSON-RPC messages into structured data
    // ========================================================================

    /**
     * @brief Parse a generic ACP message
     * @param doc JSON document to parse
     * @param message Output structure for parsed data
     * @return true if parsing succeeded, false otherwise
     */
    static bool parseMessage(const QJsonDocument &doc, ACPMessage &message);

    /**
     * @brief Parse an initialize response
     * @param doc JSON document to parse
     * @param result Output structure for agent info and capabilities
     * @return true if parsing succeeded, false otherwise
     */
    static bool parseInitializeResponse(const QJsonDocument &doc, InitializeResult &result);

    /**
     * @brief Parse a session/update notification
     * @param doc JSON document to parse
     * @param update Output structure for session update data
     * @return true if parsing succeeded, false otherwise
     */
    static bool parseSessionUpdate(const QJsonDocument &doc, SessionUpdateNotification &update);

    /**
     * @brief Parse a progress notification
     * @param doc JSON document to parse
     * @param progress Output structure for progress data
     * @return true if parsing succeeded, false otherwise
     */
    static bool parseProgressNotification(const QJsonDocument &doc, ProgressNotification &progress);

    /**
     * @brief Parse an error response
     * @param doc JSON document to parse
     * @param error Output structure for error data
     * @return true if parsing succeeded, false otherwise
     */
    static bool parseErrorResponse(const QJsonDocument &doc, ACPError &error);

    // ========================================================================
    // UTILITY METHODS
    // ========================================================================

    /**
     * @brief Generate a unique request ID
     * @return Incrementing 64-bit integer request ID
     * @note IDs start at 1 and increment monotonically per process
     */
    static qint64 generateRequestId();

    /**
     * @brief Get the supported protocol version
     * @return Protocol version string (e.g., "1")
     */
    static QString getProtocolVersion();
};

} // namespace ACP
