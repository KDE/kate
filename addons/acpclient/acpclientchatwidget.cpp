/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#include "acpclientchatwidget.h"
#include "acpchatmessagewidget.h"
#include "acpclient_debug.h"
#include "acpclientplugin.h"
#include "acpclientprotocol.h"
#include "acpclientserver.h"
#include "acpclientservermanager.h"

#include <KLocalizedString>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QScrollArea>
#include <QTimer>

#include "ui_acpclientchat.h"

ACPClientChatWidget::ACPClientChatWidget(ACPClientPlugin *plugin, KTextEditor::MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent)
    , m_plugin(plugin)
    , m_mainWindow(mainWindow)
    , m_ui(new Ui::ACPChatWidget)
{
    qCDebug(ACPCLIENT) << "ACPClientChatWidget created";

    m_ui->setupUi(this);

    // Get server manager
    if (m_plugin) {
        m_serverManager = m_plugin->serverManager();
        if (m_serverManager) {
            m_server = m_serverManager->activeServer();
        }
    }

    // Connect signals
    connect(m_ui->sendButton, &QPushButton::clicked, this, &ACPClientChatWidget::sendMessage);
    connect(m_ui->messageInput, &QLineEdit::returnPressed, this, &ACPClientChatWidget::onInputReturnPressed);
    connect(m_ui->newSessionButton, &QPushButton::clicked, this, &ACPClientChatWidget::startNewSession);
    connect(m_ui->endSessionButton, &QPushButton::clicked, this, &ACPClientChatWidget::sessionEnded);

    // Get the scroll area and message container from UI
    m_chatScrollArea = m_ui->chatScrollArea;
    m_chatDisplayContainer = m_ui->chatDisplay;
    m_chatMessagesLayout = qobject_cast<QVBoxLayout *>(m_chatDisplayContainer->layout());

    if (!m_chatMessagesLayout) {
        // Fallback if layout not found
        m_chatMessagesLayout = new QVBoxLayout(m_chatDisplayContainer);
        m_chatMessagesLayout->setSpacing(2);
        m_chatMessagesLayout->setContentsMargins(4, 4, 4, 4);
    }

    // Update UI state
    updateSessionState();
}

ACPClientChatWidget::~ACPClientChatWidget()
{
    qCDebug(ACPCLIENT) << "ACPClientChatWidget destroyed";

    // Clean up all message widgets
    clearMessages();

    delete m_ui;
}

void ACPClientChatWidget::startNewSession()
{
    qCDebug(ACPCLIENT) << "Starting new chat session";

    if (!m_serverManager) {
        appendMessage(QStringLiteral("System"), i18n("No ACP server manager available"));
        return;
    }

    ACPClientServer *server = m_serverManager->activeServer();
    if (!server) {
        appendMessage(QStringLiteral("System"), i18n("No active ACP server. Please connect to an agent first."));
        return;
    }

    m_server = server;

    // Connect to server messages first
    connect(server, &ACPClientServer::messageReceived, this, &ACPClientChatWidget::onServerMessageReceived);
    connect(server, &ACPClientServer::disconnected, this, [this]() {
        appendMessage(QStringLiteral("System"), i18n("Server disconnected"));
        setSessionId(QString());
        updateSessionState();
    });

    // Connect to sessionCreated signal to get the actual session ID
    connect(m_serverManager, &ACPClientServerManager::sessionCreated, this, [this](const QString &sessionId) {
        if (!m_sessionId.isEmpty()) {
            // Already have a session, ignore
            return;
        }
        setSessionId(sessionId);
        appendMessage(QStringLiteral("System"), i18n("New ACP session started: %1", sessionId));
        updateSessionState();
    });

    // Connect to messageReceived to show all messages in chat
    connect(m_serverManager, &ACPClientServerManager::messageReceived, this, [this](const QJsonDocument &doc) {
        // Forward to our handler
        onServerMessageReceived(doc);
    });

    // Connect to permissionRequested signal
    connect(m_serverManager, &ACPClientServerManager::permissionRequested, this, &ACPClientChatWidget::onPermissionRequested);

    // Connect permissionResponse signal to send response back to server
    connect(this, &ACPClientChatWidget::permissionResponse, this, [server](qint64 requestId, const QString &optionId) {
        if (server && server->state() == ACPClientServer::ServerState::Initialized) {
            QJsonDocument response = ACP::ACPProtocol::createPermissionResponse(requestId, optionId);
            server->sendMessage(response);
        }
    });

    // Create a new session
    m_serverManager->createSession();
    appendMessage(QStringLiteral("System"), i18n("Creating new ACP session..."));
}

void ACPClientChatWidget::setSessionId(const QString &sessionId)
{
    if (m_sessionId != sessionId) {
        m_sessionId = sessionId;
        updateSessionState();
    }
}

QString ACPClientChatWidget::sessionId() const
{
    return m_sessionId;
}

void ACPClientChatWidget::appendMessage(const QString &sender, const QString &message, bool isUser)
{
    ACPChatMessageWidget *msgWidget =
        new ACPChatMessageWidget(isUser ? ACPChatMessageWidget::MessageType::User : ACPChatMessageWidget::MessageType::Agent, m_chatDisplayContainer);

    msgWidget->setTimestamp(QDateTime::currentDateTime());
    msgWidget->setSender(isUser ? i18n("You") : sender);
    msgWidget->setContent(message);

    addMessageWidget(msgWidget);
}

void ACPClientChatWidget::addMessageWidget(ACPChatMessageWidget *widget)
{
    if (!widget || !m_chatMessagesLayout) {
        return;
    }

    widget->show();
    m_chatMessagesLayout->addWidget(widget);
    m_messageWidgets.append(widget);

    // Scroll to bottom
    QTimer::singleShot(0, this, [this]() {
        m_chatScrollArea->ensureVisible(0, m_chatDisplayContainer->height(), 0, 0);
    });
}

void ACPClientChatWidget::clearMessages()
{
    // Delete all message widgets
    for (ACPChatMessageWidget *widget : m_messageWidgets) {
        widget->deleteLater();
    }
    m_messageWidgets.clear();

    // Clear layout
    QLayoutItem *child;
    while ((child = m_chatMessagesLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
}

void ACPClientChatWidget::onPermissionRequested(qint64 requestId, const QJsonObject &toolCall, const QJsonArray &options)
{
    qCDebug(ACPCLIENT) << "Permission requested for requestId:" << requestId;

    // Get tool call ID for display
    QString toolCallId = toolCall[u"toolCallId"].toString();
    QString title = toolCall[u"title"].toString();
    QString kind = toolCall[u"kind"].toString();

    // Get the plugin to check the permission mode
    if (!m_plugin) {
        qCWarning(ACPCLIENT) << "No plugin available to check permission mode";
        // Default to deny if we can't check
        Q_EMIT permissionResponse(requestId, ACP::PERMISSION_KIND_REJECT_ONCE);
        return;
    }

    ACPClientPluginOptions::ToolCallPermission permissionMode = m_plugin->toolCallPermission();

    // If not asking each time, auto-respond
    if (permissionMode != ACPClientPluginOptions::AskEachTime) {
        QString optionId;
        if (permissionMode == ACPClientPluginOptions::AllowAll) {
            // Find an allow option, prefer allow_always then allow_once
            for (const QJsonValue &opt : options) {
                if (opt.isObject()) {
                    QJsonObject option = opt.toObject();
                    QString kind = option[u"kind"].toString();
                    if (kind == ACP::PERMISSION_KIND_ALLOW_ALWAYS) {
                        optionId = option[u"optionId"].toString();
                        break;
                    } else if (kind == ACP::PERMISSION_KIND_ALLOW_ONCE && optionId.isEmpty()) {
                        optionId = option[u"optionId"].toString();
                    }
                }
            }
            if (optionId.isEmpty()) {
                // Fallback to allow_once
                optionId = ACP::PERMISSION_KIND_ALLOW_ONCE;
            }
        } else { // DenyAll
            // Find a reject option, prefer reject_always then reject_once
            for (const QJsonValue &opt : options) {
                if (opt.isObject()) {
                    QJsonObject option = opt.toObject();
                    QString kind = option[u"kind"].toString();
                    if (kind == ACP::PERMISSION_KIND_REJECT_ALWAYS) {
                        optionId = option[u"optionId"].toString();
                        break;
                    } else if (kind == ACP::PERMISSION_KIND_REJECT_ONCE && optionId.isEmpty()) {
                        optionId = option[u"optionId"].toString();
                    }
                }
            }
            if (optionId.isEmpty()) {
                // Fallback to reject_once
                optionId = ACP::PERMISSION_KIND_REJECT_ONCE;
            }
        }
        qCDebug(ACPCLIENT) << "Auto-responding to permission request with:" << optionId;
        Q_EMIT permissionResponse(requestId, optionId);
        return;
    }

    // Ask the user - build a message to display
    QString message = i18n("The ACP agent requests permission to execute a tool call.");
    if (!title.isEmpty()) {
        message += QStringLiteral("\n\n<strong>%1</strong>").arg(title);
    }
    if (!toolCallId.isEmpty()) {
        message += QStringLiteral("\n\n[ID: %1]").arg(toolCallId);
    }
    if (!kind.isEmpty()) {
        message += QStringLiteral("\n\nKind: %1").arg(kind);
    }

    // Display the permission request in the chat
    appendMessage(i18n("ACP Agent"), message);

    // Build option names for the dialog
    QStringList optionNames;
    QMap<QString, QString> optionIdToName; // Maps optionId to display name

    for (const QJsonValue &opt : options) {
        if (opt.isObject()) {
            QJsonObject option = opt.toObject();
            QString name = option[u"name"].toString();
            QString optionId = option[u"optionId"].toString();
            optionNames.append(name);
            optionIdToName[optionId] = name;
        }
    }

    // If no options, default to allow once
    if (optionNames.isEmpty()) {
        optionNames.append(i18n("Allow once"));
        optionIdToName[ACP::PERMISSION_KIND_ALLOW_ONCE] = i18n("Allow once");
    }

    // Show dialog to user
    // We use the main window if available
    QWidget *parentWidget = m_mainWindow ? m_mainWindow->window() : this;

    // Map option names to standard buttons
    // We'll just use Yes for allow, No for reject
    bool hasAllow = false;
    bool hasReject = false;
    QString allowOptionId;
    QString rejectOptionId;

    for (const QJsonValue &opt : options) {
        if (opt.isObject()) {
            QJsonObject option = opt.toObject();
            QString kind = option[u"kind"].toString();
            QString optionId = option[u"optionId"].toString();

            if (kind == ACP::PERMISSION_KIND_ALLOW_ONCE || kind == ACP::PERMISSION_KIND_ALLOW_ALWAYS) {
                hasAllow = true;
                allowOptionId = optionId;
            } else if (kind == ACP::PERMISSION_KIND_REJECT_ONCE || kind == ACP::PERMISSION_KIND_REJECT_ALWAYS) {
                hasReject = true;
                rejectOptionId = optionId;
            }
        }
    }

    if (!hasAllow) {
        allowOptionId = ACP::PERMISSION_KIND_ALLOW_ONCE;
        hasAllow = true;
    }
    if (!hasReject) {
        rejectOptionId = ACP::PERMISSION_KIND_REJECT_ONCE;
        hasReject = true;
    }

    // Simplify: just show Yes/No dialog
    if (hasAllow && hasReject) {
        QMessageBox msgBox(parentWidget);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setWindowTitle(i18n("Tool Call Permission"));
        msgBox.setText(i18n("The ACP agent requests permission to execute a tool call."));
        if (!title.isEmpty()) {
            msgBox.setInformativeText(title);
        }
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        msgBox.setModal(true);

        int result = msgBox.exec();

        if (result == QMessageBox::Yes) {
            Q_EMIT permissionResponse(requestId, allowOptionId);
        } else {
            Q_EMIT permissionResponse(requestId, rejectOptionId);
        }
    } else if (hasAllow) {
        // Only allow option available
        Q_EMIT permissionResponse(requestId, allowOptionId);
    } else {
        // Only reject option available
        Q_EMIT permissionResponse(requestId, rejectOptionId);
    }
}

void ACPClientChatWidget::clearChat()
{
    clearMessages();
    m_messageHistory.clear();
}

void ACPClientChatWidget::setServer(ACPClientServer *server)
{
    if (m_server != server) {
        // Disconnect from old server
        if (m_server) {
            disconnect(m_server, nullptr, this, nullptr);
        }

        m_server = server;

        // Connect to new server
        if (m_server) {
            connect(m_server, &ACPClientServer::messageReceived, this, &ACPClientChatWidget::onServerMessageReceived);
            connect(m_server, &ACPClientServer::disconnected, this, [this]() {
                appendMessage(QStringLiteral("System"), i18n("Server disconnected"));
                setSessionId(QString());
                updateSessionState();
            });

            // Connect permissionResponse to send back to this server
            connect(this, &ACPClientChatWidget::permissionResponse, this, [server](qint64 requestId, const QString &optionId) {
                if (server->state() == ACPClientServer::ServerState::Initialized) {
                    QJsonDocument response = ACP::ACPProtocol::createPermissionResponse(requestId, optionId);
                    server->sendMessage(response);
                }
            });
        }
    }
}

void ACPClientChatWidget::sendMessage()
{
    QString message = m_ui->messageInput->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    // Add to history
    if (!m_messageHistory.contains(message)) {
        m_messageHistory.prepend(message);
    }
    m_historyIndex = 0;

    // Display user message
    appendMessage(i18n("You"), message, true);
    m_ui->messageInput->clear();

    // Send to ACP server
    if (!m_sessionId.isEmpty() && m_serverManager) {
        m_serverManager->sendPrompt(m_sessionId, message);
        Q_EMIT messageSent(m_sessionId, message);
    } else {
        appendMessage(QStringLiteral("System"), i18n("No active session. Please start a new session first."));
    }
}

void ACPClientChatWidget::onInputReturnPressed()
{
    sendMessage();
}

void ACPClientChatWidget::onServerMessageReceived(const QJsonDocument &message)
{
    if (!message.isObject()) {
        return;
    }

    QJsonObject obj = message.object();

    // Handle error responses
    if (obj.contains(u"error")) {
        QJsonObject errorObj = obj[u"error"].toObject();
        QString errorMsg = errorObj[u"message"].toString();
        QString errorCode = QString::number(errorObj[u"code"].toInt());
        appendMessage(QStringLiteral("System"), i18n("Error [%1]: %2", errorCode, errorMsg));
        return;
    }

    // Handle successful responses
    if (obj.contains(u"id") && !obj.contains(u"method")) {
        if (obj.contains(u"result")) {
            QJsonObject result = obj[u"result"].toObject();
            if (result.contains(u"stopReason")) {
                QString stopReason = result[u"stopReason"].toString();
                appendMessage(i18n("ACP Agent"), i18n("Turn completed: %1", stopReason));
            }
        }
        return;
    }

    // Handle session update notifications
    if (obj.contains(u"method") && obj[u"method"].toString() == ACP::NOTIFICATION_SESSION_UPDATE) {
        if (obj.contains(u"params") && obj[u"params"].isObject()) {
            QJsonObject params = obj[u"params"].toObject();
            QString sessionId = params[u"sessionId"].toString();

            // Only process messages for our current session
            if (sessionId != m_sessionId) {
                return;
            }

            // Check for sessionUpdate type
            if (params.contains(u"update") && params[u"update"].isObject()) {
                QJsonObject update = params[u"update"].toObject();
                QString updateType = update[u"sessionUpdate"].toString();

                // Handle different update types
                if (updateType == ACP::SESSION_UPDATE_AGENT_MESSAGE_CHUNK) {
                    handleAgentMessageChunk(update);
                } else if (updateType == ACP::SESSION_UPDATE_PLAN) {
                    handlePlanUpdate(update);
                } else if (updateType == ACP::SESSION_UPDATE_TOOL_CALL) {
                    handleToolCallUpdate(update);
                } else if (updateType == ACP::SESSION_UPDATE_TOOL_CALL_UPDATE) {
                    handleToolCallStatusUpdate(update);
                } else if (updateType == ACP::SESSION_UPDATE_USAGE_UPDATE) {
                    handleUsageUpdate(update);
                }
            }
            // Fallback for legacy status-based messages
            else if (params.contains(u"status")) {
                QString status = params[u"status"].toString();
                QString msg = params[u"message"].toString();

                if (status == QStringLiteral("idle")) {
                    QString stopReason = params[u"stopReason"].toString();
                    if (!stopReason.isEmpty()) {
                        appendMessage(i18n("ACP Agent"), msg + QStringLiteral(" (Reason: ") + stopReason + QStringLiteral(")"));
                    } else {
                        appendMessage(i18n("ACP Agent"), msg);
                    }
                } else if (status == QStringLiteral("working") || status == QStringLiteral("completed")) {
                    appendMessage(i18n("ACP Agent"), msg);
                } else if (status == QStringLiteral("error")) {
                    appendMessage(i18n("ACP Agent"), i18n("Error: %1", msg));
                }
            }
        }
    }

    // Handle progress notifications
    if (obj.contains(u"method") && obj[u"method"].toString() == ACP::NOTIFICATION_PROGRESS) {
        if (obj.contains(u"params") && obj[u"params"].isObject()) {
            QJsonObject params = obj[u"params"].toObject();
            if (params.contains(u"value") && params[u"value"].isObject()) {
                QJsonObject value = params[u"value"].toObject();
                if (value.contains(u"message")) {
                    appendMessage(i18n("ACP Agent"), value[u"message"].toString());
                }
            }
        }
    }
}

void ACPClientChatWidget::handleAgentMessageChunk(const QJsonObject &update)
{
    if (update.contains(u"content") && update[u"content"].isObject()) {
        QJsonObject content = update[u"content"].toObject();
        QString messageId = update[u"messageId"].toString();

        if (content.contains(u"type") && content[u"type"].toString() == QStringLiteral("text")) {
            QString text = content[u"text"].toString();

            // Check if this is a new message or continuation
            static QString lastMessageId;
            if (messageId != lastMessageId && !messageId.isEmpty()) {
                // New message
                lastMessageId = messageId;
                ACPChatMessageWidget *msgWidget = new ACPChatMessageWidget(ACPChatMessageWidget::MessageType::Agent, m_chatDisplayContainer);
                msgWidget->setTimestamp(QDateTime::currentDateTime());
                msgWidget->setSender(i18n("Agent"));
                msgWidget->setMessageId(messageId);
                msgWidget->setContent(text);
                addMessageWidget(msgWidget);
            } else if (!messageId.isEmpty()) {
                // Continuation - try to find the last agent message and append
                if (!m_messageWidgets.isEmpty()) {
                    ACPChatMessageWidget *lastWidget = m_messageWidgets.last();
                    if (lastWidget->type() == ACPChatMessageWidget::MessageType::Agent && lastWidget->messageId() == messageId) {
                        // Append to existing message
                        QString existingContent = lastWidget->content();
                        lastWidget->setContent(existingContent + QStringLiteral(" ") + text);
                        return;
                    }
                }
                // Fallback: create new message
                ACPChatMessageWidget *msgWidget = new ACPChatMessageWidget(ACPChatMessageWidget::MessageType::Agent, m_chatDisplayContainer);
                msgWidget->setTimestamp(QDateTime::currentDateTime());
                msgWidget->setSender(i18n("Agent"));
                msgWidget->setContent(text);
                addMessageWidget(msgWidget);
            } else {
                // No message ID - create new message
                ACPChatMessageWidget *msgWidget = new ACPChatMessageWidget(ACPChatMessageWidget::MessageType::Agent, m_chatDisplayContainer);
                msgWidget->setTimestamp(QDateTime::currentDateTime());
                msgWidget->setSender(i18n("Agent"));
                msgWidget->setContent(text);
                addMessageWidget(msgWidget);
            }
        }
    }
}

void ACPClientChatWidget::handlePlanUpdate(const QJsonObject &update)
{
    if (update.contains(u"entries") && update[u"entries"].isArray()) {
        QJsonArray entries = update[u"entries"].toArray();

        ACPChatMessageWidget *msgWidget = new ACPChatMessageWidget(ACPChatMessageWidget::MessageType::Plan, m_chatDisplayContainer);
        msgWidget->setTimestamp(QDateTime::currentDateTime());
        msgWidget->setSender(i18n("Agent"));

        // Add each plan entry
        for (const QJsonValue &entryValue : entries) {
            if (entryValue.isObject()) {
                QJsonObject entry = entryValue.toObject();
                QString content = entry[u"content"].toString();
                QString priority = entry[u"priority"].toString();
                QString status = entry[u"status"].toString();
                msgWidget->addPlanEntry(content, priority, status);
            }
        }

        addMessageWidget(msgWidget);
    }
}

void ACPClientChatWidget::handleToolCallUpdate(const QJsonObject &update)
{
    QString toolCallId = update[u"toolCallId"].toString();
    QString title = update[u"title"].toString();
    QString kind = update[u"kind"].toString();
    QString status = update[u"status"].toString();

    ACPChatMessageWidget *msgWidget = new ACPChatMessageWidget(ACPChatMessageWidget::MessageType::ToolCall, m_chatDisplayContainer);
    msgWidget->setTimestamp(QDateTime::currentDateTime());
    msgWidget->setSender(i18n("Agent"));
    msgWidget->setToolCallInfo(toolCallId, title, kind, status);

    addMessageWidget(msgWidget);
}

void ACPClientChatWidget::handleToolCallStatusUpdate(const QJsonObject &update)
{
    QString toolCallId = update[u"toolCallId"].toString();
    QString status = update[u"status"].toString();
    QString contentText;

    // Check for content
    if (update.contains(u"content") && update[u"content"].isArray()) {
        QJsonArray contentArray = update[u"content"].toArray();
        for (const QJsonValue &contentValue : contentArray) {
            if (contentValue.isObject()) {
                QJsonObject content = contentValue.toObject();
                if (content.contains(u"content") && content[u"content"].isObject()) {
                    QJsonObject innerContent = content[u"content"].toObject();
                    if (innerContent.contains(u"type") && innerContent[u"type"].toString() == QStringLiteral("text")) {
                        contentText = innerContent[u"text"].toString();
                        break;
                    }
                }
            }
        }
    }

    ACPChatMessageWidget *msgWidget = new ACPChatMessageWidget(ACPChatMessageWidget::MessageType::ToolCallUpdate, m_chatDisplayContainer);
    msgWidget->setTimestamp(QDateTime::currentDateTime());
    msgWidget->setSender(i18n("Agent"));
    msgWidget->setToolCallStatus(toolCallId, status, contentText);

    addMessageWidget(msgWidget);
}

void ACPClientChatWidget::handleUsageUpdate(const QJsonObject &update)
{
    qint64 used = update[u"used"].toInt();
    qint64 size = update[u"size"].toInt();
    double cost = 0.0;
    QString currency;

    // Handle cost if present
    if (update.contains(u"cost") && update[u"cost"].isObject()) {
        QJsonObject costObj = update[u"cost"].toObject();
        if (costObj.contains(u"amount") && costObj.contains(u"currency")) {
            cost = costObj[u"amount"].toDouble();
            currency = costObj[u"currency"].toString();
        }
    }

    ACPChatMessageWidget *msgWidget = new ACPChatMessageWidget(ACPChatMessageWidget::MessageType::Usage, m_chatDisplayContainer);
    msgWidget->setTimestamp(QDateTime::currentDateTime());
    msgWidget->setSender(i18n("System"));
    msgWidget->setUsageInfo(used, size, cost, currency);

    addMessageWidget(msgWidget);
}

void ACPClientChatWidget::updateSessionState()
{
    if (m_sessionId.isEmpty()) {
        m_ui->sessionLabel->setText(i18n("ACP Chat: No active session"));
        m_ui->endSessionButton->setEnabled(false);
    } else {
        m_ui->sessionLabel->setText(i18n("ACP Chat: Session %1", m_sessionId));
        m_ui->endSessionButton->setEnabled(true);
    }
}

#include "moc_acpclientchatwidget.cpp"
