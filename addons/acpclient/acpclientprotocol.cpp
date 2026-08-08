/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
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
    request[jsonrpcVersionKey()] = jsonrpcVersionValue();
    request[jsonrpcId()] = QJsonValue(static_cast<qint64>(requestId));
    request[jsonrpcMethod()] = methodInitialize();

    QJsonObject paramsObj;
    // Protocol version should be integer 1 for v1
    paramsObj[u"protocolVersion"] = params.protocolVersion;

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
        sessionConfigObj[u"boolean"] = QJsonObject(); // Empty object indicates support
        QJsonObject sessionObj;
        sessionObj[u"configOptions"] = sessionConfigObj;
        clientCapabilitiesObj[u"session"] = sessionObj;
    }

    if (!clientCapabilitiesObj.isEmpty()) {
        paramsObj[u"clientCapabilities"] = clientCapabilitiesObj;
    }

    // Client info
    QJsonObject clientInfo;
    clientInfo[u"name"] = params.clientInfo.name;
    clientInfo[u"title"] = params.clientInfo.title;
    clientInfo[u"version"] = params.clientInfo.version;
    if (!clientInfo.isEmpty()) {
        paramsObj[u"clientInfo"] = clientInfo;
    }

    request[jsonrpcParams()] = paramsObj;
    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createAuthLoginRequest(const AuthLoginParams &params, qint64 requestId)
{
    QJsonObject request;
    request[jsonrpcVersionKey()] = jsonrpcVersionValue();
    request[jsonrpcId()] = QJsonValue(static_cast<qint64>(requestId));
    request[jsonrpcMethod()] = methodAuthLogin();

    QJsonObject paramsObj;
    paramsObj[u"providerId"] = params.providerId;
    paramsObj[u"providerData"] = params.providerData;

    request[jsonrpcParams()] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionNewRequest(const SessionNewParams &params, qint64 requestId)
{
    QJsonObject request;
    request[jsonrpcVersionKey()] = jsonrpcVersionValue();
    request[jsonrpcId()] = QJsonValue(static_cast<qint64>(requestId));
    request[jsonrpcMethod()] = methodSessionNew();

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

    request[jsonrpcParams()] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionLoadRequest(const SessionLoadParams &params, qint64 requestId)
{
    QJsonObject request;
    request[jsonrpcVersionKey()] = jsonrpcVersionValue();
    request[jsonrpcId()] = QJsonValue(static_cast<qint64>(requestId));
    request[jsonrpcMethod()] = methodSessionLoad();

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

    request[jsonrpcParams()] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionResumeRequest(const SessionResumeParams &params, qint64 requestId)
{
    QJsonObject request;
    request[jsonrpcVersionKey()] = jsonrpcVersionValue();
    request[jsonrpcId()] = QJsonValue(static_cast<qint64>(requestId));
    request[jsonrpcMethod()] = methodSessionResume();

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

    request[jsonrpcParams()] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionCloseRequest(const SessionCloseParams &params, qint64 requestId)
{
    QJsonObject request;
    request[jsonrpcVersionKey()] = jsonrpcVersionValue();
    request[jsonrpcId()] = QJsonValue(static_cast<qint64>(requestId));
    request[jsonrpcMethod()] = methodSessionClose();

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = params.sessionId;

    request[jsonrpcParams()] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionListRequest(const ListSessionsRequest &params, qint64 requestId)
{
    QJsonObject request;
    request[jsonrpcVersionKey()] = jsonrpcVersionValue();
    request[jsonrpcId()] = QJsonValue(static_cast<qint64>(requestId));
    request[jsonrpcMethod()] = methodSessionList();

    QJsonObject paramsObj;

    // Add optional fields only if they have non-default values
    // limit: only include if >= 0 (vibe-acp expects no limit by default)
    if (params.limit >= 0) {
        paramsObj[u"limit"] = params.limit;
    }

    // offset: only include if > 0
    if (params.offset > 0) {
        paramsObj[u"offset"] = params.offset;
    }

    // Always include params object, even if empty, to satisfy strict servers
    request[jsonrpcParams()] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionDeleteRequest(const SessionDeleteParams &params, qint64 requestId)
{
    QJsonObject request;
    request[jsonrpcVersionKey()] = jsonrpcVersionValue();
    request[jsonrpcId()] = QJsonValue(static_cast<qint64>(requestId));
    request[jsonrpcMethod()] = methodSessionDelete();

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = params.sessionId;

    request[jsonrpcParams()] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionPromptRequest(const SessionPromptParams &params, qint64 requestId)
{
    QJsonObject request;
    request[jsonrpcVersionKey()] = jsonrpcVersionValue();
    request[jsonrpcId()] = QJsonValue(static_cast<qint64>(requestId));
    request[jsonrpcMethod()] = methodSessionPrompt();

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = params.sessionId;

    // Build prompt as array of ContentBlocks
    QJsonArray promptArray;
    for (const ContentBlock &block : params.prompt) {
        QJsonObject contentBlock;
        contentBlock[u"type"] = block.type;

        if (block.type == contentTypeText()) {
            contentBlock[u"text"] = block.text;
        } else if (block.type == contentTypeResource() || block.type == contentTypeResourceLink()) {
            contentBlock[u"resource"] = block.resource;
        } else if (block.type == contentTypeImage()) {
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
        } else if (block.type == contentTypeAudio()) {
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

    request[jsonrpcParams()] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionCancelRequest(const QString &sessionId, qint64 requestId)
{
    QJsonObject request;
    request[jsonrpcVersionKey()] = jsonrpcVersionValue();
    request[jsonrpcId()] = QJsonValue(static_cast<qint64>(requestId));
    request[jsonrpcMethod()] = methodSessionCancel();

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = sessionId;

    request[jsonrpcParams()] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createToolsListRequest(qint64 requestId)
{
    QJsonObject request;
    request[jsonrpcVersionKey()] = jsonrpcVersionValue();
    request[jsonrpcId()] = QJsonValue(static_cast<qint64>(requestId));
    request[jsonrpcMethod()] = methodToolsList();

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createToolsCallRequest(const QString &toolId, const QJsonObject &arguments, qint64 requestId)
{
    QJsonObject request;
    request[jsonrpcVersionKey()] = jsonrpcVersionValue();
    request[jsonrpcId()] = QJsonValue(static_cast<qint64>(requestId));
    request[jsonrpcMethod()] = methodToolsCall();

    QJsonObject paramsObj;
    paramsObj[u"toolIdentifier"] = toolId;
    paramsObj[u"arguments"] = arguments;

    request[jsonrpcParams()] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionUpdateNotification(const SessionUpdateNotification &notification)
{
    QJsonObject notif;
    notif[jsonrpcVersionKey()] = jsonrpcVersionValue();
    notif[jsonrpcMethod()] = notificationSessionUpdate();

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = notification.sessionId;
    paramsObj[u"update"] = notification.update;

    notif[jsonrpcParams()] = paramsObj;

    return QJsonDocument(notif);
}

QJsonDocument ACPProtocol::createProgressNotification(const ProgressNotification &notification)
{
    QJsonObject notif;
    notif[jsonrpcVersionKey()] = jsonrpcVersionValue();
    notif[jsonrpcMethod()] = notificationProgress();

    QJsonObject paramsObj;
    paramsObj[u"token"] = notification.token;
    paramsObj[u"value"] = notification.value;

    notif[jsonrpcParams()] = paramsObj;

    return QJsonDocument(notif);
}

QJsonDocument ACPProtocol::createCancelRequestNotification(const CancelRequestNotification &notification)
{
    QJsonObject notif;
    notif[jsonrpcVersionKey()] = jsonrpcVersionValue();
    notif[jsonrpcMethod()] = notificationCancellation();

    QJsonObject paramsObj;
    paramsObj[u"id"] = notification.id;

    notif[jsonrpcParams()] = paramsObj;

    return QJsonDocument(notif);
}

QJsonDocument ACPProtocol::createPermissionResponse(qint64 requestId, const QString &optionId)
{
    QJsonObject response;
    response[jsonrpcVersionKey()] = jsonrpcVersionValue();
    response[jsonrpcId()] = QJsonValue(static_cast<qint64>(requestId));

    QJsonObject resultObj;
    QJsonObject outcomeObj;
    outcomeObj[u"outcome"] = permissionOutcomeSelected();
    outcomeObj[u"optionId"] = optionId;
    resultObj[u"outcome"] = outcomeObj;

    response[jsonrpcResult()] = resultObj;

    return QJsonDocument(response);
}

QJsonDocument ACPProtocol::createPermissionResponseCancelled(qint64 requestId)
{
    QJsonObject response;
    response[jsonrpcVersionKey()] = jsonrpcVersionValue();
    response[jsonrpcId()] = QJsonValue(static_cast<qint64>(requestId));

    QJsonObject resultObj;
    QJsonObject outcomeObj;
    outcomeObj[u"outcome"] = permissionOutcomeCancelled();
    resultObj[u"outcome"] = outcomeObj;

    response[jsonrpcResult()] = resultObj;

    return QJsonDocument(response);
}

bool ACPProtocol::parseMessage(const QJsonDocument &doc, ACPMessage &message)
{
    if (!doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();

    // Check if it's a JSON-RPC message
    if (obj.contains(jsonrpcVersionKey())) {
        QString version = obj[jsonrpcVersionKey()].toString();
        if (version != jsonrpcVersionValue()) {
            return false;
        }
    }

    // Parse common fields
    if (obj.contains(jsonrpcId())) {
        QJsonValue idValue = obj[jsonrpcId()];
        if (idValue.isDouble()) {
            message.id = static_cast<qint64>(idValue.toInteger());
        } else {
            message.id = 0;
        }
    }

    if (obj.contains(jsonrpcMethod())) {
        message.method = obj[jsonrpcMethod()].toString();
    }

    if (obj.contains(jsonrpcParams())) {
        message.params = obj[jsonrpcParams()].toObject();
    }

    // Check if it's a response
    if (obj.contains(jsonrpcResult())) {
        message.result = obj[jsonrpcResult()].toObject();
        message.isResponse = true;
    } else if (obj.contains(jsonrpcError())) {
        QJsonObject errorObj = obj[jsonrpcError()].toObject();
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
    if (!obj.contains(jsonrpcResult())) {
        return false;
    }

    QJsonObject resultObj = obj[jsonrpcResult()].toObject();

    // Get agent info
    if (resultObj.contains(u"agentInfo") && resultObj[u"agentInfo"].isObject()) {
        QJsonObject agentInfo = resultObj[u"agentInfo"].toObject();
        result.agentInfo.name = agentInfo[u"name"].toString();
        result.agentInfo.title = agentInfo[u"title"].toString();
        result.agentInfo.version = agentInfo[u"version"].toString();
    }

    // Get auth methods
    if (resultObj.contains(u"authMethods") && resultObj[u"authMethods"].isArray()) {
        result.authMethods = resultObj[u"authMethods"].toArray();
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
        result.capabilities.supportsAuthentication = !result.authMethods.isEmpty();

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
            const QJsonArray versions = caps[u"supportedProtocolVersions"].toArray();
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
    if (!obj.contains(jsonrpcMethod()) || obj[jsonrpcMethod()].toString() != notificationSessionUpdate()) {
        return false;
    }

    if (!obj.contains(jsonrpcParams())) {
        return false;
    }

    QJsonObject params = obj[jsonrpcParams()].toObject();

    // sessionId is required
    if (!params.contains(u"sessionId")) {
        return false;
    }
    update.sessionId = params[u"sessionId"].toString();

    // update object is required
    if (!params.contains(u"update") || !params[u"update"].isObject()) {
        return false;
    }
    update.update = params[u"update"].toObject();

    return true;
}

bool ACPProtocol::parseProgressNotification(const QJsonDocument &doc, ProgressNotification &progress)
{
    if (!doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();

    // Check if it's a progress notification
    if (!obj.contains(jsonrpcMethod()) || obj[jsonrpcMethod()].toString() != notificationProgress()) {
        return false;
    }

    if (!obj.contains(jsonrpcParams())) {
        return false;
    }

    QJsonObject params = obj[jsonrpcParams()].toObject();

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
    if (!obj.contains(jsonrpcError())) {
        return false;
    }

    QJsonObject errorObj = obj[jsonrpcError()].toObject();

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
    return protocolVersion();
}

} // namespace ACP
