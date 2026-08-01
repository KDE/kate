/*
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpclientprotocol.h"

#include <KAboutData>
#include <KLocalizedString>

namespace ACP
{

QJsonDocument ACPProtocol::createInitializeRequest(const InitializeParams &params, qint64 requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = QJsonValue(static_cast<qint64>(requestId));
    request[JSONRPC_METHOD] = METHOD_INITIALIZE;

    QJsonObject paramsObj;
    // Protocol version should be integer 1 for v1
    paramsObj[u"protocolVersion"] = PROTOCOL_VERSION_INT;

    // Client capabilities
    QJsonObject clientCapabilitiesObj;

    // File system capabilities
    QJsonObject fsCapabilities;
    fsCapabilities[u"readTextFile"] = params.clientCapabilities.fs.readTextFile;
    fsCapabilities[u"writeTextFile"] = params.clientCapabilities.fs.writeTextFile;
    if (!fsCapabilities.isEmpty()) {
        clientCapabilitiesObj[u"fs"] = fsCapabilities;
    }

    // Terminal capabilities
    if (params.clientCapabilities.terminal) {
        clientCapabilitiesObj[u"terminal"] = true;
    }

    // Session config options
    if (params.clientCapabilities.sessionConfigOptionsBoolean.supported) {
        QJsonObject sessionConfigObj;
        sessionConfigObj[u"boolean"] = true;
        QJsonObject sessionObj;
        sessionObj[u"configOptions"] = sessionConfigObj;
        clientCapabilitiesObj[u"session"] = sessionObj;
    }

    if (!clientCapabilitiesObj.isEmpty()) {
        paramsObj[u"clientCapabilities"] = clientCapabilitiesObj;
    }

    QJsonObject clientInfo;
    clientInfo[u"name"] = QStringLiteral("kate");
    clientInfo[u"title"] = i18n("Kate ACP Client");
    clientInfo[u"version"] = KAboutData::applicationData().version();
    if (!clientInfo.isEmpty()) {
        paramsObj[u"clientInfo"] = clientInfo;
    }

    request[JSONRPC_PARAMS] = paramsObj;
    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createAuthLoginRequest(const AuthLoginParams &params, qint64 requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = QJsonValue(static_cast<qint64>(requestId));
    request[JSONRPC_METHOD] = METHOD_AUTH_LOGIN;

    QJsonObject paramsObj;
    paramsObj[u"providerId"] = params.providerId;
    paramsObj[u"providerData"] = params.providerData;

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionNewRequest(const SessionNewParams &params, qint64 requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = QJsonValue(static_cast<qint64>(requestId));
    request[JSONRPC_METHOD] = METHOD_SESSION_NEW;

    QJsonObject paramsObj;

    if (!params.cwd.isEmpty()) {
        paramsObj[u"cwd"] = params.cwd;
    }

    // mcpServers - always send as array per protocol spec
    if (params.mcpServers.isEmpty()) {
        paramsObj[u"mcpServers"] = QJsonArray();
    } else {
        paramsObj[u"mcpServers"] = params.mcpServers;
    }

    // additionalDirectories - optional
    if (!params.additionalDirectories.isEmpty()) {
        paramsObj[u"additionalDirectories"] = params.additionalDirectories;
    }

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionLoadRequest(const SessionLoadParams &params, qint64 requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = QJsonValue(static_cast<qint64>(requestId));
    request[JSONRPC_METHOD] = METHOD_SESSION_LOAD;

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = params.sessionId;

    if (!params.cwd.isEmpty()) {
        paramsObj[u"cwd"] = params.cwd;
    }

    // mcpServers - always send as array
    if (params.mcpServers.isEmpty()) {
        paramsObj[u"mcpServers"] = QJsonArray();
    } else {
        paramsObj[u"mcpServers"] = params.mcpServers;
    }

    // additionalDirectories - optional
    if (!params.additionalDirectories.isEmpty()) {
        paramsObj[u"additionalDirectories"] = params.additionalDirectories;
    }

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionResumeRequest(const SessionResumeParams &params, qint64 requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = QJsonValue(static_cast<qint64>(requestId));
    request[JSONRPC_METHOD] = METHOD_SESSION_RESUME;

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = params.sessionId;

    if (!params.cwd.isEmpty()) {
        paramsObj[u"cwd"] = params.cwd;
    }

    // mcpServers - always send as array
    if (params.mcpServers.isEmpty()) {
        paramsObj[u"mcpServers"] = QJsonArray();
    } else {
        paramsObj[u"mcpServers"] = params.mcpServers;
    }

    // additionalDirectories - optional
    if (!params.additionalDirectories.isEmpty()) {
        paramsObj[u"additionalDirectories"] = params.additionalDirectories;
    }

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionCloseRequest(const SessionCloseParams &params, qint64 requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = QJsonValue(static_cast<qint64>(requestId));
    request[JSONRPC_METHOD] = METHOD_SESSION_CLOSE;

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = params.sessionId;

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionListRequest(qint64 requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = QJsonValue(static_cast<qint64>(requestId));
    request[JSONRPC_METHOD] = METHOD_SESSION_LIST;

    // session/list typically doesn't require parameters, send empty object
    // Some servers (like vibe-acp) expect either no params or a valid ListSessionsRequest
    QJsonObject params;
    request[JSONRPC_PARAMS] = params;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionDeleteRequest(const SessionDeleteParams &params, qint64 requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = QJsonValue(static_cast<qint64>(requestId));
    request[JSONRPC_METHOD] = METHOD_SESSION_DELETE;

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = params.sessionId;

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionPromptRequest(const SessionPromptParams &params, qint64 requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = QJsonValue(static_cast<qint64>(requestId));
    request[JSONRPC_METHOD] = METHOD_SESSION_PROMPT;

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = params.sessionId;

    // Build prompt as array of ContentBlocks
    QJsonArray promptArray;
    for (const ContentBlock &block : params.prompt) {
        QJsonObject contentBlock;
        contentBlock[u"type"] = block.type;

        if (block.type == CONTENT_TYPE_TEXT) {
            contentBlock[u"text"] = block.text;
        } else if (block.type == CONTENT_TYPE_RESOURCE || block.type == CONTENT_TYPE_RESOURCE_LINK) {
            contentBlock[u"resource"] = block.resource;
        } else if (block.type == CONTENT_TYPE_IMAGE) {
            // Image content block
            if (!block.mimeType.isEmpty()) {
                contentBlock[u"mimeType"] = block.mimeType;
            }
            if (!block.text.isEmpty()) {
                contentBlock[u"data"] = block.text; // base64 data
            }
            if (!block.resource.isEmpty()) {
                contentBlock[u"resource"] = block.resource;
            }
        } else if (block.type == CONTENT_TYPE_AUDIO) {
            // Audio content block
            if (!block.mimeType.isEmpty()) {
                contentBlock[u"mimeType"] = block.mimeType;
            }
            if (!block.text.isEmpty()) {
                contentBlock[u"data"] = block.text; // base64 data
            }
            if (!block.resource.isEmpty()) {
                contentBlock[u"resource"] = block.resource;
            }
        }

        promptArray.append(contentBlock);
    }

    paramsObj[u"prompt"] = promptArray;

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionCancelRequest(const QString &sessionId, qint64 requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = QJsonValue(static_cast<qint64>(requestId));
    request[JSONRPC_METHOD] = METHOD_SESSION_CANCEL;

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = sessionId;

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createToolsListRequest(qint64 requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = QJsonValue(static_cast<qint64>(requestId));
    request[JSONRPC_METHOD] = METHOD_TOOLS_LIST;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createToolsCallRequest(const QString &toolId, const QJsonObject &arguments, qint64 requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = QJsonValue(static_cast<qint64>(requestId));
    request[JSONRPC_METHOD] = METHOD_TOOLS_CALL;

    QJsonObject paramsObj;
    paramsObj[u"toolIdentifier"] = toolId;
    paramsObj[u"arguments"] = arguments;

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionUpdateNotification(const SessionUpdateNotification &notification)
{
    QJsonObject notif;
    notif[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    notif[JSONRPC_METHOD] = NOTIFICATION_SESSION_UPDATE;

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = notification.sessionId;
    paramsObj[u"status"] = notification.status;

    if (!notification.message.isEmpty()) {
        paramsObj[u"message"] = notification.message;
    }
    if (!notification.stopReason.isEmpty()) {
        paramsObj[u"stopReason"] = notification.stopReason;
    }

    notif[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(notif);
}

QJsonDocument ACPProtocol::createProgressNotification(const ProgressNotification &notification)
{
    QJsonObject notif;
    notif[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    notif[JSONRPC_METHOD] = NOTIFICATION_PROGRESS;

    QJsonObject paramsObj;
    paramsObj[u"token"] = notification.token;
    paramsObj[u"value"] = notification.value;

    notif[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(notif);
}

QJsonDocument ACPProtocol::createCancelRequestNotification(const CancelRequestNotification &notification)
{
    QJsonObject notif;
    notif[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    notif[JSONRPC_METHOD] = NOTIFICATION_CANCELLATION;

    QJsonObject paramsObj;
    paramsObj[u"id"] = notification.id;

    notif[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(notif);
}

QJsonDocument ACPProtocol::createPermissionResponse(qint64 requestId, const QString &optionId)
{
    QJsonObject response;
    response[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    response[JSONRPC_ID] = QJsonValue(static_cast<qint64>(requestId));

    QJsonObject resultObj;
    QJsonObject outcomeObj;
    outcomeObj[u"outcome"] = PERMISSION_OUTCOME_SELECTED;
    outcomeObj[u"optionId"] = optionId;
    resultObj[u"outcome"] = outcomeObj;

    response[JSONRPC_RESULT] = resultObj;

    return QJsonDocument(response);
}

QJsonDocument ACPProtocol::createPermissionResponseCancelled(qint64 requestId)
{
    QJsonObject response;
    response[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    response[JSONRPC_ID] = QJsonValue(static_cast<qint64>(requestId));

    QJsonObject resultObj;
    QJsonObject outcomeObj;
    outcomeObj[u"outcome"] = PERMISSION_OUTCOME_CANCELLED;
    resultObj[u"outcome"] = outcomeObj;

    response[JSONRPC_RESULT] = resultObj;

    return QJsonDocument(response);
}

bool ACPProtocol::parseMessage(const QJsonDocument &doc, ACPMessage &message)
{
    if (!doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();

    // Check if it's a JSON-RPC message
    if (obj.contains(JSONRPC_VERSION_KEY)) {
        QString version = obj[JSONRPC_VERSION_KEY].toString();
        if (version != JSONRPC_VERSION_VALUE) {
            return false;
        }
    }

    // Parse common fields
    if (obj.contains(JSONRPC_ID)) {
        QJsonValue idValue = obj[JSONRPC_ID];
        if (idValue.isDouble()) {
            message.id = static_cast<qint64>(idValue.toInteger());
        } else {
            message.id = 0;
        }
    }

    if (obj.contains(JSONRPC_METHOD)) {
        message.method = obj[JSONRPC_METHOD].toString();
    }

    if (obj.contains(JSONRPC_PARAMS)) {
        message.params = obj[JSONRPC_PARAMS].toObject();
    }

    // Check if it's a response
    if (obj.contains(JSONRPC_RESULT)) {
        message.result = obj[JSONRPC_RESULT].toObject();
        message.isResponse = true;
    } else if (obj.contains(JSONRPC_ERROR)) {
        QJsonObject errorObj = obj[JSONRPC_ERROR].toObject();
        if (errorObj.contains(u"code")) {
            message.error.code = static_cast<ErrorCode>(errorObj[u"code"].toInt());
        }
        if (errorObj.contains(u"message")) {
            message.error.message = errorObj[u"message"].toString();
        }
        if (errorObj.contains(u"data")) {
            message.error.data = errorObj[u"data"].toObject();
        }
        message.isResponse = true;
    }

    // Check if it's a notification (no id field, has method)
    if (message.id == 0 && !message.method.isEmpty()) {
        message.isNotification = true;
    }

    // Return true if it's a valid message (has method) or a response (has result/error)
    return !message.method.isEmpty() || message.isResponse;
}

bool ACPProtocol::parseInitializeResponse(const QJsonDocument &doc, InitializeResult &result)
{
    if (!doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();

    // Check if it's a valid response
    if (!obj.contains(JSONRPC_RESULT)) {
        return false;
    }

    QJsonObject resultObj = obj[JSONRPC_RESULT].toObject();

    // Try to get agent info from agentInfo object or fallback to individual fields
    if (resultObj.contains(u"agentInfo") && resultObj[u"agentInfo"].isObject()) {
        QJsonObject agentInfo = resultObj[u"agentInfo"].toObject();
        result.agentName = agentInfo[u"name"].toString();
        if (result.agentName.isEmpty()) {
            result.agentName = agentInfo[u"title"].toString();
        }
        result.agentVersion = agentInfo[u"version"].toString();
    } else {
        // Fallback to individual fields
        if (resultObj.contains(u"agentName")) {
            result.agentName = resultObj[u"agentName"].toString();
        }
        if (resultObj.contains(u"agentVersion")) {
            result.agentVersion = resultObj[u"agentVersion"].toString();
        }
    }

    // Protocol version can be a string or number
    if (resultObj.contains(u"protocolVersion")) {
        QJsonValue versionValue = resultObj[u"protocolVersion"];
        if (versionValue.isString()) {
            result.protocolVersion = versionValue.toString();
        } else if (versionValue.isDouble()) {
            result.protocolVersion = QString::number(versionValue.toInt());
        }
    }

    // Try to get capabilities from agentCapabilities (v1 spec)
    if (resultObj.contains(u"agentCapabilities") && resultObj[u"agentCapabilities"].isObject()) {
        QJsonObject agentCaps = resultObj[u"agentCapabilities"].toObject();

        // Parse all capabilities according to v1 spec
        result.capabilities.loadSession = agentCaps[u"loadSession"].toBool();
        result.capabilities.supportsSessions = true; // All agents must support sessions
        result.capabilities.supportsTools = true;
        result.capabilities.supportsProgress = true;
        result.capabilities.supportsAuthentication = resultObj.contains(u"authMethods") && !resultObj[u"authMethods"].toArray().isEmpty();

        // Prompt capabilities
        if (agentCaps.contains(u"promptCapabilities") && agentCaps[u"promptCapabilities"].isObject()) {
            QJsonObject promptCaps = agentCaps[u"promptCapabilities"].toObject();
            result.capabilities.promptCapabilities.image = promptCaps[u"image"].toBool();
            result.capabilities.promptCapabilities.audio = promptCaps[u"audio"].toBool();
            result.capabilities.promptCapabilities.embeddedContext = promptCaps[u"embeddedContext"].toBool();
        }

        // MCP capabilities
        if (agentCaps.contains(u"mcpCapabilities") && agentCaps[u"mcpCapabilities"].isObject()) {
            QJsonObject mcpCaps = agentCaps[u"mcpCapabilities"].toObject();
            result.capabilities.mcpCapabilities.http = mcpCaps[u"http"].toBool();
            result.capabilities.mcpCapabilities.sse = mcpCaps[u"sse"].toBool();
        }

        // Auth capabilities
        if (agentCaps.contains(u"auth") && agentCaps[u"auth"].isObject()) {
            QJsonObject authCaps = agentCaps[u"auth"].toObject();
            result.capabilities.auth.logout = authCaps[u"logout"].toBool();
        }

        // Session capabilities
        if (agentCaps.contains(u"sessionCapabilities") && agentCaps[u"sessionCapabilities"].isObject()) {
            QJsonObject sessionCaps = agentCaps[u"sessionCapabilities"].toObject();
            result.capabilities.sessionCapabilities.resume = sessionCaps[u"resume"].toBool();
            result.capabilities.sessionCapabilities.close = sessionCaps[u"close"].toBool();
            result.capabilities.sessionCapabilities.deleteSession = sessionCaps.contains(u"delete");
            result.capabilities.sessionCapabilities.additionalDirectories = sessionCaps[u"additionalDirectories"].toBool();
        }

        // Protocol version
        if (resultObj.contains(u"protocolVersion")) {
            QJsonValue versionValue = resultObj[u"protocolVersion"];
            if (versionValue.isDouble()) {
                result.protocolVersion = QString::number(versionValue.toInt());
            } else if (versionValue.isString()) {
                result.protocolVersion = versionValue.toString();
            }
        }

        // Custom capabilities
        if (agentCaps.contains(u"_meta")) {
            result.capabilities.customCapabilities = agentCaps[u"_meta"].toObject();
        }
    } else if (resultObj.contains(u"capabilities") && resultObj[u"capabilities"].isObject()) {
        // Fallback for older implementations
        QJsonObject caps = resultObj[u"capabilities"].toObject();
        result.capabilities.loadSession = caps[u"supportsSessions"].toBool();
        result.capabilities.supportsSessions = caps[u"supportsSessions"].toBool();
        result.capabilities.supportsTools = caps[u"supportsTools"].toBool();
        result.capabilities.supportsProgress = caps[u"supportsProgress"].toBool();
        result.capabilities.supportsAuthentication = caps[u"supportsAuthentication"].toBool();

        if (caps.contains(u"supportedProtocolVersions")) {
            QJsonArray versions = caps[u"supportedProtocolVersions"].toArray();
            for (const QJsonValue &v : versions) {
                result.capabilities.supportedProtocolVersions.append(v.toString());
            }
        }

        if (caps.contains(u"customCapabilities")) {
            result.capabilities.customCapabilities = caps[u"customCapabilities"].toObject();
        }
    } else {
        // Default capabilities for backward compatibility
        result.capabilities.supportsSessions = true;
        result.capabilities.supportsTools = true;
    }

    return true;
}

bool ACPProtocol::parseSessionUpdate(const QJsonDocument &doc, SessionUpdateNotification &update)
{
    if (!doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();

    // Check if it's a notification
    if (!obj.contains(JSONRPC_METHOD) || obj[JSONRPC_METHOD].toString() != NOTIFICATION_SESSION_UPDATE) {
        return false;
    }

    if (!obj.contains(JSONRPC_PARAMS)) {
        return false;
    }

    QJsonObject params = obj[JSONRPC_PARAMS].toObject();

    if (params.contains(u"sessionId")) {
        update.sessionId = params[u"sessionId"].toString();
    }
    if (params.contains(u"status")) {
        update.status = params[u"status"].toString();
    }
    if (params.contains(u"message")) {
        update.message = params[u"message"].toString();
    }
    if (params.contains(u"stopReason")) {
        update.stopReason = params[u"stopReason"].toString();
    }

    return true;
}

bool ACPProtocol::parseProgressNotification(const QJsonDocument &doc, ProgressNotification &progress)
{
    if (!doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();

    // Check if it's a progress notification
    if (!obj.contains(JSONRPC_METHOD) || obj[JSONRPC_METHOD].toString() != NOTIFICATION_PROGRESS) {
        return false;
    }

    if (!obj.contains(JSONRPC_PARAMS)) {
        return false;
    }

    QJsonObject params = obj[JSONRPC_PARAMS].toObject();

    if (params.contains(u"token")) {
        progress.token = params[u"token"].toString();
    }
    if (params.contains(u"value")) {
        progress.value = params[u"value"].toObject();
    }

    return true;
}

bool ACPProtocol::parseErrorResponse(const QJsonDocument &doc, ACPError &error)
{
    if (!doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();

    // Check if it's an error response
    if (!obj.contains(JSONRPC_ERROR)) {
        return false;
    }

    QJsonObject errorObj = obj[JSONRPC_ERROR].toObject();

    if (errorObj.contains(u"code")) {
        error.code = static_cast<ErrorCode>(errorObj[u"code"].toInt());
    } else {
        error.code = ErrorCode::UnknownError;
    }

    if (errorObj.contains(u"message")) {
        error.message = errorObj[u"message"].toString();
    }

    if (errorObj.contains(u"data")) {
        error.data = errorObj[u"data"].toObject();
    }

    return true;
}

qint64 ACPProtocol::generateRequestId()
{
    // Use an incrementing 64-bit integer as the request ID
    static qint64 nextId = 1;
    return nextId++;
}

QString ACPProtocol::getProtocolVersion()
{
    return PROTOCOL_VERSION;
}

} // namespace ACP
