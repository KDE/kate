/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#include "acpclientprotocol.h"

#include <QUuid>

namespace ACP
{

QJsonDocument ACPProtocol::createInitializeRequest(const InitializeParams &params, const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_INITIALIZE;

    QJsonObject paramsObj;
    paramsObj[u"clientName"] = params.clientName;
    paramsObj[u"clientVersion"] = params.clientVersion;
    paramsObj[u"protocolVersion"] = params.protocolVersion;

    QJsonObject capabilitiesObj;
    capabilitiesObj[u"supportsSessions"] = params.capabilities.supportsSessions;
    capabilitiesObj[u"supportsTools"] = params.capabilities.supportsTools;
    capabilitiesObj[u"supportsProgress"] = params.capabilities.supportsProgress;
    capabilitiesObj[u"supportsAuthentication"] = params.capabilities.supportsAuthentication;
    capabilitiesObj[u"supportedProtocolVersions"] = QJsonArray::fromStringList(params.capabilities.supportedProtocolVersions);

    if (!params.capabilities.customCapabilities.isEmpty()) {
        capabilitiesObj[u"customCapabilities"] = params.capabilities.customCapabilities;
    }

    paramsObj[u"capabilities"] = capabilitiesObj;

    if (!params.metadata.isEmpty()) {
        paramsObj[u"metadata"] = params.metadata;
    }

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createAuthLoginRequest(const AuthLoginParams &params, const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_AUTH_LOGIN;

    QJsonObject paramsObj;
    paramsObj[u"providerId"] = params.providerId;
    paramsObj[u"providerData"] = params.providerData;

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionNewRequest(const SessionNewParams &params, const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_SESSION_NEW;

    QJsonObject paramsObj;
    if (!params.metadata.isEmpty()) {
        paramsObj[u"metadata"] = params.metadata;
    }

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionResumeRequest(const SessionResumeParams &params, const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_SESSION_RESUME;

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = params.sessionId;

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionListRequest(const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_SESSION_LIST;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionDeleteRequest(const QString &sessionId, const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_SESSION_DELETE;

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = sessionId;

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionPromptRequest(const SessionPromptParams &params, const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_SESSION_PROMPT;

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = params.sessionId;
    paramsObj[u"message"] = params.message;

    if (!params.metadata.isEmpty()) {
        paramsObj[u"metadata"] = params.metadata;
    }

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionCancelRequest(const QString &sessionId, const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_SESSION_CANCEL;

    QJsonObject paramsObj;
    paramsObj[u"sessionId"] = sessionId;

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createToolsListRequest(const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_TOOLS_LIST;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createToolsCallRequest(const QString &toolId, const QJsonObject &arguments, const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION_KEY] = JSONRPC_VERSION_VALUE;
    request[JSONRPC_ID] = requestId;
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
    if (!notification.metadata.isEmpty()) {
        paramsObj[u"metadata"] = notification.metadata;
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
        message.id = obj[JSONRPC_ID].toString();
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
    if (message.id.isEmpty() && !message.method.isEmpty()) {
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

    if (resultObj.contains(u"metadata")) {
        result.metadata = resultObj[u"metadata"].toObject();
    }

    // Try to get capabilities from capabilities or agentCapabilities
    if (resultObj.contains(u"capabilities") && resultObj[u"capabilities"].isObject()) {
        QJsonObject caps = resultObj[u"capabilities"].toObject();
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
    } else if (resultObj.contains(u"agentCapabilities") && resultObj[u"agentCapabilities"].isObject()) {
        // Fallback to agentCapabilities
        QJsonObject agentCaps = resultObj[u"agentCapabilities"].toObject();
        // Map agentCapabilities to our capabilities structure
        result.capabilities.supportsSessions = agentCaps.contains(u"loadSession");
        result.capabilities.supportsTools = true; // agentCapabilities implies tool support
        result.capabilities.supportsProgress = true;
        result.capabilities.supportsAuthentication = resultObj.contains(u"authMethods");
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
    if (params.contains(u"metadata")) {
        update.metadata = params[u"metadata"].toObject();
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

QString ACPProtocol::generateRequestId()
{
    return QUuid::createUuid().toString().mid(1, 36); // Remove braces
}

QString ACPProtocol::getProtocolVersion()
{
    return PROTOCOL_VERSION;
}

} // namespace ACP
