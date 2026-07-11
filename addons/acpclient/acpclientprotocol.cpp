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
    request[JSONRPC_VERSION] = JSONRPC_VERSION;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_INITIALIZE;

    QJsonObject paramsObj;
    paramsObj["clientName"] = params.clientName;
    paramsObj["clientVersion"] = params.clientVersion;

    QJsonObject capabilitiesObj;
    capabilitiesObj["supportsSessions"] = params.capabilities.supportsSessions;
    capabilitiesObj["supportsTools"] = params.capabilities.supportsTools;
    capabilitiesObj["supportsProgress"] = params.capabilities.supportsProgress;
    capabilitiesObj["supportsAuthentication"] = params.capabilities.supportsAuthentication;
    capabilitiesObj["supportedProtocolVersions"] = QJsonArray::fromStringList(params.capabilities.supportedProtocolVersions);

    if (!params.capabilities.customCapabilities.isEmpty()) {
        capabilitiesObj["customCapabilities"] = params.capabilities.customCapabilities;
    }

    paramsObj["capabilities"] = capabilitiesObj;

    if (!params.metadata.isEmpty()) {
        paramsObj["metadata"] = params.metadata;
    }

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createAuthLoginRequest(const AuthLoginParams &params, const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION] = JSONRPC_VERSION;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_AUTH_LOGIN;

    QJsonObject paramsObj;
    paramsObj["providerId"] = params.providerId;
    paramsObj["providerData"] = params.providerData;

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionNewRequest(const SessionNewParams &params, const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION] = JSONRPC_VERSION;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_SESSION_NEW;

    QJsonObject paramsObj;
    if (!params.metadata.isEmpty()) {
        paramsObj["metadata"] = params.metadata;
    }

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionResumeRequest(const SessionResumeParams &params, const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION] = JSONRPC_VERSION;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_SESSION_RESUME;

    QJsonObject paramsObj;
    paramsObj["sessionId"] = params.sessionId;

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionListRequest(const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION] = JSONRPC_VERSION;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_SESSION_LIST;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionDeleteRequest(const QString &sessionId, const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION] = JSONRPC_VERSION;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_SESSION_DELETE;

    QJsonObject paramsObj;
    paramsObj["sessionId"] = sessionId;

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionPromptRequest(const SessionPromptParams &params, const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION] = JSONRPC_VERSION;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_SESSION_PROMPT;

    QJsonObject paramsObj;
    paramsObj["sessionId"] = params.sessionId;
    paramsObj["message"] = params.message;

    if (!params.metadata.isEmpty()) {
        paramsObj["metadata"] = params.metadata;
    }

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionCancelRequest(const QString &sessionId, const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION] = JSONRPC_VERSION;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_SESSION_CANCEL;

    QJsonObject paramsObj;
    paramsObj["sessionId"] = sessionId;

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createToolsListRequest(const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION] = JSONRPC_VERSION;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_TOOLS_LIST;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createToolsCallRequest(const QString &toolId, const QJsonObject &arguments, const QString &requestId)
{
    QJsonObject request;
    request[JSONRPC_VERSION] = JSONRPC_VERSION;
    request[JSONRPC_ID] = requestId;
    request[JSONRPC_METHOD] = METHOD_TOOLS_CALL;

    QJsonObject paramsObj;
    paramsObj["toolIdentifier"] = toolId;
    paramsObj["arguments"] = arguments;

    request[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(request);
}

QJsonDocument ACPProtocol::createSessionUpdateNotification(const SessionUpdateNotification &notification)
{
    QJsonObject notif;
    notif[JSONRPC_VERSION] = JSONRPC_VERSION;
    notif[JSONRPC_METHOD] = NOTIFICATION_SESSION_UPDATE;

    QJsonObject paramsObj;
    paramsObj["sessionId"] = notification.sessionId;
    paramsObj["status"] = notification.status;

    if (!notification.message.isEmpty()) {
        paramsObj["message"] = notification.message;
    }
    if (!notification.metadata.isEmpty()) {
        paramsObj["metadata"] = notification.metadata;
    }
    if (!notification.stopReason.isEmpty()) {
        paramsObj["stopReason"] = notification.stopReason;
    }

    notif[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(notif);
}

QJsonDocument ACPProtocol::createProgressNotification(const ProgressNotification &notification)
{
    QJsonObject notif;
    notif[JSONRPC_VERSION] = JSONRPC_VERSION;
    notif[JSONRPC_METHOD] = NOTIFICATION_PROGRESS;

    QJsonObject paramsObj;
    paramsObj["token"] = notification.token;
    paramsObj["value"] = notification.value;

    notif[JSONRPC_PARAMS] = paramsObj;

    return QJsonDocument(notif);
}

QJsonDocument ACPProtocol::createCancelRequestNotification(const CancelRequestNotification &notification)
{
    QJsonObject notif;
    notif[JSONRPC_VERSION] = JSONRPC_VERSION;
    notif[JSONRPC_METHOD] = NOTIFICATION_CANCELLATION;

    QJsonObject paramsObj;
    paramsObj["id"] = notification.id;

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
    if (obj.contains(JSONRPC_VERSION)) {
        QString version = obj[JSONRPC_VERSION].toString();
        if (version != JSONRPC_VERSION) {
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
        if (errorObj.contains("code")) {
            message.error.code = static_cast<ErrorCode>(errorObj["code"].toInt());
        }
        if (errorObj.contains("message")) {
            message.error.message = errorObj["message"].toString();
        }
        if (errorObj.contains("data")) {
            message.error.data = errorObj["data"].toObject();
        }
        message.isResponse = true;
    }

    // Check if it's a notification (no id field, has method)
    if (message.id.isEmpty() && !message.method.isEmpty()) {
        message.isNotification = true;
    }

    return !message.method.isEmpty();
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

    if (resultObj.contains("agentName")) {
        result.agentName = resultObj["agentName"].toString();
    }
    if (resultObj.contains("agentVersion")) {
        result.agentVersion = resultObj["agentVersion"].toString();
    }
    if (resultObj.contains("protocolVersion")) {
        result.protocolVersion = resultObj["protocolVersion"].toString();
    }
    if (resultObj.contains("metadata")) {
        result.metadata = resultObj["metadata"].toObject();
    }

    if (resultObj.contains("capabilities")) {
        QJsonObject caps = resultObj["capabilities"].toObject();
        result.capabilities.supportsSessions = caps["supportsSessions"].toBool();
        result.capabilities.supportsTools = caps["supportsTools"].toBool();
        result.capabilities.supportsProgress = caps["supportsProgress"].toBool();
        result.capabilities.supportsAuthentication = caps["supportsAuthentication"].toBool();

        if (caps.contains("supportedProtocolVersions")) {
            QJsonArray versions = caps["supportedProtocolVersions"].toArray();
            for (const QJsonValue &v : versions) {
                result.capabilities.supportedProtocolVersions.append(v.toString());
            }
        }

        if (caps.contains("customCapabilities")) {
            result.capabilities.customCapabilities = caps["customCapabilities"].toObject();
        }
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

    if (params.contains("sessionId")) {
        update.sessionId = params["sessionId"].toString();
    }
    if (params.contains("status")) {
        update.status = params["status"].toString();
    }
    if (params.contains("message")) {
        update.message = params["message"].toString();
    }
    if (params.contains("metadata")) {
        update.metadata = params["metadata"].toObject();
    }
    if (params.contains("stopReason")) {
        update.stopReason = params["stopReason"].toString();
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

    if (params.contains("token")) {
        progress.token = params["token"].toString();
    }
    if (params.contains("value")) {
        progress.value = params["value"].toObject();
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

    if (errorObj.contains("code")) {
        error.code = static_cast<ErrorCode>(errorObj["code"].toInt());
    } else {
        error.code = ErrorCode::UnknownError;
    }

    if (errorObj.contains("message")) {
        error.message = errorObj["message"].toString();
    }

    if (errorObj.contains("data")) {
        error.data = errorObj["data"].toObject();
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