/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

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

#include <QClipboard>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QScrollArea>
#include <QScrollBar>
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
            // Set initial server if one is active
            ACPClientServer *initialServer = m_serverManager->activeServer();
            if (initialServer) {
                setServer(initialServer);
                setupServerConnections();
            }
            // Connect to activeServerChanged to update action states when server initialization changes
            connect(m_serverManager, &ACPClientServerManager::activeServerChanged, this, [this](ACPClientServer *server) {
                Q_UNUSED(server);
                updateActionStates();
            });
        }
    }

    // Get status label
    m_statusLabel = m_ui->statusLabel;

    // Connect signals
    connect(m_ui->sendButton, &QPushButton::clicked, this, &ACPClientChatWidget::sendMessage);
    connect(m_ui->newSessionButton, &QPushButton::clicked, this, &ACPClientChatWidget::startNewSession);
    connect(m_ui->endSessionButton, &QPushButton::clicked, this, &ACPClientChatWidget::endSession);

    // Set up QComboBox as editable with completer and history model
    QComboBox *inputCombo = m_ui->messageInput;
    inputCombo->setEditable(true);
    inputCombo->setInsertPolicy(QComboBox::NoInsert);
    inputCombo->setMinimumContentsLength(20);

    // Set up model with message history
    m_historyModel = new QStringListModel(m_messageHistory, this);
    inputCombo->setModel(m_historyModel);

    // Set up completer
    m_completer = new QCompleter(m_historyModel, inputCombo);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchContains);
    inputCombo->setCompleter(m_completer);

    // Connect to returnPressed signal from the line edit inside the combo box
    connect(inputCombo->lineEdit(), &QLineEdit::returnPressed, this, &ACPClientChatWidget::onInputReturnPressed);

    // Get the scroll area and message container from UI
    m_chatScrollArea = m_ui->chatScrollArea;
    m_chatScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_chatDisplayContainer = m_ui->chatDisplay;
    m_chatDisplayContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_chatMessagesLayout = qobject_cast<QVBoxLayout *>(m_chatDisplayContainer->layout());

    if (!m_chatMessagesLayout) {
        // Fallback if layout not found
        m_chatMessagesLayout = new QVBoxLayout(m_chatDisplayContainer);
        m_chatMessagesLayout->setSpacing(2);
        m_chatMessagesLayout->setContentsMargins(4, 4, 4, 4);
    }

    // Install event filter on chat display container for context menu
    m_chatDisplayContainer->installEventFilter(this);

    // Update UI state - initialize actions as disabled (no server yet)
    updateSessionState();
    updateActionStates();
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

    // Set the server and establish connections
    setServer(server);
    setupServerConnections();

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

    // Create a new session
    m_serverManager->createSession();
    appendMessage(QStringLiteral("System"), i18n("Creating new ACP session..."));
}

void ACPClientChatWidget::setSessionId(const QString &sessionId)
{
    if (m_sessionId != sessionId) {
        m_sessionId = sessionId;
        // Clear stored tool calls when session changes
        m_toolCalls.clear();
        updateSessionState();
    }
}

QString ACPClientChatWidget::sessionId() const
{
    return m_sessionId;
}

void ACPClientChatWidget::initializeWithSession(const QString &sessionId)
{
    if (!m_serverManager) {
        appendMessage(QStringLiteral("System"), i18n("No ACP server manager available"));
        return;
    }

    ACPClientServer *server = m_serverManager->activeServer();
    if (!server) {
        appendMessage(QStringLiteral("System"), i18n("No active ACP server. Please connect to an agent first."));
        return;
    }

    // Set the server and establish connections
    setServer(server);
    setupServerConnections();

    // Set the session ID
    setSessionId(sessionId);
    appendMessage(QStringLiteral("System"), i18n("Using session: %1", sessionId));
    updateSessionState();
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

    // Find and temporarily remove the stretch if it exists
    QLayoutItem *stretchItem = nullptr;
    int stretchIndex = -1;
    for (int i = 0; i < m_chatMessagesLayout->count(); ++i) {
        QLayoutItem *item = m_chatMessagesLayout->itemAt(i);
        if (item && item->spacerItem()) {
            stretchItem = item;
            stretchIndex = i;
            break;
        }
    }

    if (stretchIndex >= 0) {
        stretchItem = m_chatMessagesLayout->takeAt(stretchIndex);
    }

    // Add the widget
    m_chatMessagesLayout->addWidget(widget);

    // Re-add the stretch at the end
    if (stretchItem) {
        m_chatMessagesLayout->addItem(stretchItem);
    } else {
        // No stretch found, add one
        m_chatMessagesLayout->addStretch();
    }

    widget->show();
    m_messageWidgets.append(widget);

    // Scroll to the bottom of the chat display
    QTimer::singleShot(0, this, [this]() {
        QScrollBar *vScrollBar = m_chatScrollArea->verticalScrollBar();
        if (vScrollBar) {
            vScrollBar->setValue(vScrollBar->maximum());
        }
    });
}

void ACPClientChatWidget::clearMessages()
{
    // Delete all message widgets
    for (ACPChatMessageWidget *widget : m_messageWidgets) {
        widget->deleteLater();
    }
    m_messageWidgets.clear();

    // Clear layout but preserve the stretch
    QLayoutItem *stretchItem = nullptr;

    // First, find and save the stretch item
    for (int i = 0; i < m_chatMessagesLayout->count(); ++i) {
        QLayoutItem *item = m_chatMessagesLayout->itemAt(i);
        if (item && item->spacerItem()) {
            stretchItem = item;
            break;
        }
    }

    // Clear all items
    QLayoutItem *child;
    while ((child = m_chatMessagesLayout->takeAt(0)) != nullptr) {
        if (child->widget() && child != stretchItem) {
            child->widget()->deleteLater();
        }
        if (child != stretchItem) {
            delete child;
        }
    }

    // Re-add the stretch if we found one, otherwise create new one
    if (stretchItem) {
        m_chatMessagesLayout->addItem(stretchItem);
    } else {
        m_chatMessagesLayout->addStretch();
    }
}

void ACPClientChatWidget::updateLastUserMessageStatus(MessageStatus status)
{
    // No conversion needed - both use the same MessageStatus enum now

    // Find the last user message and update its status
    for (auto it = m_messageWidgets.rbegin(); it != m_messageWidgets.rend(); ++it) {
        if ((*it)->type() == ACPChatMessageWidget::MessageType::User) {
            (*it)->setStatus(status);
            break;
        }
    }
}

QString ACPClientChatWidget::getAllChatText() const
{
    QString allText;
    for (ACPChatMessageWidget *widget : m_messageWidgets) {
        if (!allText.isEmpty()) {
            allText += QLatin1String("\n\n");
        }
        // Format: [timestamp] sender: content
        allText +=
            QStringLiteral("[%1] %2: %3")
                .arg(widget->findChild<QLabel *>(QStringLiteral("timestampLabel")) ? widget->findChild<QLabel *>(QStringLiteral("timestampLabel"))->text()
                                                                                   : QString(),
                     widget->findChild<QLabel *>(QStringLiteral("senderLabel")) ? widget->findChild<QLabel *>(QStringLiteral("senderLabel"))->text()
                                                                                : QString(),
                     widget->content());
    }
    return allText;
}

void ACPClientChatWidget::copyChatText()
{
    QString text = getAllChatText();
    if (!text.isEmpty()) {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(text);
    }
}

void ACPClientChatWidget::updateStatus(const QString &text)
{
    if (m_statusLabel) {
        m_statusLabel->setText(text);
    }
}

void ACPClientChatWidget::onPermissionRequested(qint64 requestId, const QJsonObject &toolCall, const QJsonArray &options)
{
    qCDebug(ACPCLIENT) << "Permission requested for requestId:" << requestId << "with" << options.size() << "options";

    // Get toolCallId from the permission request
    QString toolCallId = toolCall[u"toolCallId"].toString();

    // Try to look up the full tool call details from our stored map
    QJsonObject fullToolCall = toolCall;
    if (!toolCallId.isEmpty() && m_toolCalls.contains(toolCallId)) {
        fullToolCall = m_toolCalls[toolCallId];
    } else if (!toolCallId.isEmpty()) {
        qCDebug(ACPCLIENT) << "Tool call" << toolCallId << "not found in storage, using permission request data";
    }

    // Get all fields from the toolCall (either from storage or from the request itself)
    QString title = fullToolCall[u"title"].toString();
    QString kind = fullToolCall[u"kind"].toString();
    QString toolIdentifier = fullToolCall[u"toolIdentifier"].toString();

    // Also check for common tool call fields
    QString toolName = fullToolCall[u"name"].toString();
    QString identifier = fullToolCall[u"identifier"].toString();
    QString action = fullToolCall[u"action"].toString();

    // Check for tool name in _meta object
    QString metaToolName;
    if (fullToolCall.contains(u"_meta") && fullToolCall[u"_meta"].isObject()) {
        QJsonObject meta = fullToolCall[u"_meta"].toObject();
        metaToolName = meta[u"tool_name"].toString();
        // Also check for effect_kind which describes the operation type
        QString effectKind = meta[u"effect_kind"].toString();
        if (!effectKind.isEmpty() && toolName.isEmpty()) {
            toolName = effectKind;
        }
    }

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
                    QString optKind = option[u"kind"].toString();
                    if (optKind == ACP::PERMISSION_KIND_ALLOW_ALWAYS) {
                        optionId = option[u"optionId"].toString();
                        break;
                    } else if (optKind == ACP::PERMISSION_KIND_ALLOW_ONCE && optionId.isEmpty()) {
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
                    QString optKind = option[u"kind"].toString();
                    if (optKind == ACP::PERMISSION_KIND_REJECT_ALWAYS) {
                        optionId = option[u"optionId"].toString();
                        break;
                    } else if (optKind == ACP::PERMISSION_KIND_REJECT_ONCE && optionId.isEmpty()) {
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

    // Parse all permission options from the server
    QList<ACPChatMessageWidget::PermissionOption> permissionOptions;
    for (const QJsonValue &opt : options) {
        if (opt.isObject()) {
            QJsonObject option = opt.toObject();
            ACPChatMessageWidget::PermissionOption permOpt;
            permOpt.optionId = option[u"optionId"].toString();
            permOpt.name = option[u"name"].toString();
            permOpt.kind = option[u"kind"].toString();
            permissionOptions.append(permOpt);
        }
    }

    // Build the full tool call description from all available fields
    QString toolCallDescription;
    QString argumentsStr;

    // Determine the tool name to display - use toolIdentifier first, then try other fields
    QString toolDisplayName;
    if (!toolIdentifier.isEmpty()) {
        toolDisplayName = toolIdentifier;
    } else if (!metaToolName.isEmpty()) {
        toolDisplayName = metaToolName;
    } else if (!identifier.isEmpty()) {
        toolDisplayName = identifier;
    } else if (!toolName.isEmpty()) {
        toolDisplayName = toolName;
    } else if (!kind.isEmpty()) {
        toolDisplayName = kind;
    } else if (!title.isEmpty()) {
        toolDisplayName = title;
    } else if (!action.isEmpty()) {
        toolDisplayName = action;
    }

    // Extract arguments - check multiple possible locations
    QJsonObject argumentsObj;
    if (fullToolCall.contains(u"arguments") && fullToolCall[u"arguments"].isObject()) {
        argumentsObj = fullToolCall[u"arguments"].toObject();
    } else if (fullToolCall.contains(u"params") && fullToolCall[u"params"].isObject()) {
        argumentsObj = fullToolCall[u"params"].toObject();
    }

    // Also check for locations (file paths being accessed)
    QStringList locationPaths;
    if (fullToolCall.contains(u"locations") && fullToolCall[u"locations"].isArray()) {
        QJsonArray locations = fullToolCall[u"locations"].toArray();
        for (const QJsonValue &loc : locations) {
            if (loc.isObject()) {
                QJsonObject locObj = loc.toObject();
                QString path = locObj[u"path"].toString();
                if (!path.isEmpty()) {
                    locationPaths.append(path);
                }
            }
        }
    }

    // Check for content field (might contain the actual content or command)
    QString content = fullToolCall[u"content"].toString();
    QString textContent = fullToolCall[u"text"].toString();

    // Build arguments string
    if (!content.isEmpty()) {
        argumentsStr = content;
    } else if (!textContent.isEmpty()) {
        argumentsStr = textContent;
    } else if (fullToolCall.contains(u"rawInput") && fullToolCall[u"rawInput"].isObject()) {
        // For shell tools, extract the command from rawInput
        QJsonObject rawInput = fullToolCall[u"rawInput"].toObject();
        if (rawInput.contains(u"command") && rawInput[u"command"].isString()) {
            argumentsStr = rawInput[u"command"].toString();
        }
    } else if (!title.isEmpty() && title.contains(u":")) {
        // Extract command from title if in "tool: command" format
        int colonPos = title.indexOf(u":");
        if (colonPos >= 0 && colonPos < title.length() - 1) {
            argumentsStr = title.mid(colonPos + 1).trimmed();
        }
    } else if (!argumentsObj.isEmpty()) {
        // For bash/shell tools, extract the command directly
        if (argumentsObj.contains(u"command")) {
            argumentsStr = argumentsObj[u"command"].toString();
        } else {
            // For other tools, show the full arguments as JSON
            argumentsStr = QString::fromUtf8(QJsonDocument(argumentsObj).toJson());
            if (argumentsStr == QStringLiteral("{}")) {
                argumentsStr.clear();
            }
        }
    } else if (!locationPaths.isEmpty()) {
        // If we have location paths but no arguments, use them
        if (locationPaths.size() == 1) {
            argumentsStr = locationPaths.first();
        } else if (locationPaths.size() > 1) {
            argumentsStr = locationPaths.join(QStringLiteral(", "));
        }
    }

    // Construct the full tool call description
    if (!toolDisplayName.isEmpty()) {
        toolCallDescription = toolDisplayName;
        if (!argumentsStr.isEmpty()) {
            toolCallDescription += QStringLiteral(" ") + argumentsStr;
        }
    } else if (!argumentsStr.isEmpty()) {
        // If we have arguments but no tool name, show just the arguments
        toolCallDescription = argumentsStr;
    } else if (!toolCallId.isEmpty()) {
        // Fallback to tool call ID
        toolCallDescription = toolCallId;
    } else {
        toolCallDescription = QStringLiteral("Unknown tool");
    }

    // Log a warning if we still can't identify the tool
    if (toolCallDescription == QStringLiteral("Unknown tool")) {
        qCWarning(ACPCLIENT) << "Permission request: Could not extract tool information. Tool call ID:" << toolCallId
                             << "Stored tool calls:" << m_toolCalls.keys() << "Full toolCall:" << QJsonDocument(fullToolCall).toJson();
    }

    qCDebug(ACPCLIENT) << "Permission request - toolCallDescription:" << toolCallDescription << "toolDisplayName:" << toolDisplayName
                       << "argumentsStr:" << argumentsStr;

    // Create a permission request widget inline in the chat
    ACPChatMessageWidget *permissionWidget = new ACPChatMessageWidget(ACPChatMessageWidget::MessageType::PermissionRequest, m_chatDisplayContainer);
    permissionWidget->setTimestamp(QDateTime::currentDateTime());
    permissionWidget->setSender(i18n("ACP Agent"));

    // Use the new method to support all permission options
    // Pass the tool name and command separately for better display
    permissionWidget->setPermissionRequestWithOptions(requestId, title, toolDisplayName, argumentsStr, permissionOptions);

    // Connect the widget's signal to our handler
    connect(permissionWidget, &ACPChatMessageWidget::permissionResponse, this, [this](qint64 reqId, const QString &optionId) {
        Q_EMIT permissionResponse(reqId, optionId);
    });

    addMessageWidget(permissionWidget);
}

void ACPClientChatWidget::clearChat()
{
    clearMessages();
    m_messageHistory.clear();
    if (m_historyModel) {
        m_historyModel->setStringList(m_messageHistory);
    }
    m_toolCalls.clear();
    updateStatus(QString());
}

void ACPClientChatWidget::setServer(ACPClientServer *server)
{
    if (m_server != server) {
        // Disconnect from old server
        if (m_server) {
            disconnect(m_server, nullptr, this, nullptr);
            // Also disconnect permissionResponse connections that were set up with the old server
            disconnect(this, &ACPClientChatWidget::permissionResponse, this, nullptr);
        }

        m_server = server;

        // Update action states with new server
        updateActionStates();

        // Connect to new server
        if (m_server) {
            // Note: messageReceived is connected via serverManager in setupServerConnections()
            // to avoid duplicate messages
            connect(m_server, &ACPClientServer::disconnected, this, [this]() {
                appendMessage(QStringLiteral("System"), i18n("Server disconnected"));
                setSessionId(QString());
                updateSessionState();
            });

            // Connect to server state changes and initialization to update action enablement
            connect(m_server, &ACPClientServer::stateChanged, this, [this](ACPClientServer::ServerState state) {
                Q_UNUSED(state);
                updateActionStates();
            });
            connect(m_server, &ACPClientServer::initialized, this, [this](const ACP::InitializeResult &result) {
                Q_UNUSED(result);
                updateActionStates();
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

void ACPClientChatWidget::setupServerConnections()
{
    if (!m_serverManager || !m_server) {
        return;
    }

    // Disconnect any existing server manager connections to avoid duplicates
    disconnect(m_serverManager, nullptr, this, nullptr);

    // Connect to messageReceived to show all messages in chat
    connect(m_serverManager, &ACPClientServerManager::messageReceived, this, [this](const QJsonDocument &doc) {
        // Forward to our handler
        onServerMessageReceived(doc);
    });

    // Connect to permissionRequested signal
    connect(m_serverManager, &ACPClientServerManager::permissionRequested, this, &ACPClientChatWidget::onPermissionRequested);

    // Connect to activeServerChanged to update action states when server initialization changes
    connect(m_serverManager, &ACPClientServerManager::activeServerChanged, this, [this](ACPClientServer *server) {
        Q_UNUSED(server);
        updateActionStates();
    });

    // Connect to sessionLoaded signal to update state
    connect(m_serverManager, &ACPClientServerManager::sessionLoaded, this, [this](const QString &sessionId) {
        if (m_sessionId == sessionId || m_sessionId.isEmpty()) {
            appendMessage(QStringLiteral("System"), i18n("Session loaded: %1", sessionId));
            updateSessionState();
        }
    });

    // Connect to sessionResumed signal to update state
    connect(m_serverManager, &ACPClientServerManager::sessionResumed, this, [this](const QString &sessionId) {
        if (m_sessionId == sessionId || m_sessionId.isEmpty()) {
            appendMessage(QStringLiteral("System"), i18n("Session resumed: %1", sessionId));
            updateSessionState();
        }
    });
}

void ACPClientChatWidget::sendMessage()
{
    QString message = m_ui->messageInput->currentText().trimmed();
    if (message.isEmpty()) {
        return;
    }

    // Add to history
    if (!m_messageHistory.contains(message)) {
        m_messageHistory.prepend(message);
        // Update the completer model
        if (m_historyModel) {
            m_historyModel->setStringList(m_messageHistory);
        }
    }

    // Display user message
    appendMessage(i18n("You"), message, true);
    m_ui->messageInput->clearEditText();

    // Mark the user message as running (prompt turn started)
    updateLastUserMessageStatus(MessageStatus::Running);

    // Send to ACP server
    if (!m_sessionId.isEmpty() && m_serverManager) {
        m_serverManager->sendPrompt(m_sessionId, message);
        Q_EMIT messageSent(m_sessionId, message);
    } else {
        // No active session - mark as error
        updateLastUserMessageStatus(MessageStatus::Error);
        appendMessage(QStringLiteral("System"), i18n("No active session. Please start a new session first."));
    }
}

void ACPClientChatWidget::onInputReturnPressed()
{
    sendMessage();
}

void ACPClientChatWidget::endSession()
{
    if (!m_serverManager) {
        appendMessage(QStringLiteral("System"), i18n("No ACP server manager available"));
        return;
    }

    if (m_sessionId.isEmpty()) {
        appendMessage(QStringLiteral("System"), i18n("No active session to end"));
        return;
    }

    // Close the current session via server manager
    m_serverManager->closeSession(m_sessionId);

    // Clear the session and update UI
    setSessionId(QString());
    updateSessionState();
    updateActionStates();
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

        // Mark the last user message as error
        updateLastUserMessageStatus(MessageStatus::Error);
        return;
    }

    // Handle successful responses
    if (obj.contains(u"id") && !obj.contains(u"method")) {
        if (obj.contains(u"result")) {
            QJsonObject result = obj[u"result"].toObject();
            if (result.contains(u"stopReason")) {
                QString stopReason = result[u"stopReason"].toString();

                // Mark the last user message as completed or error based on stopReason
                if (stopReason == QStringLiteral("cancelled")) {
                    updateLastUserMessageStatus(MessageStatus::Cancelled);
                } else if (stopReason == QStringLiteral("refusal")) {
                    updateLastUserMessageStatus(MessageStatus::Error);
                } else {
                    // end_turn, max_tokens, max_turn_requests - all indicate completion
                    updateLastUserMessageStatus(MessageStatus::Completed);
                }
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

                qCDebug(ACPCLIENT) << "Session update type:" << updateType << "== TOOL_CALL?" << (updateType == ACP::SESSION_UPDATE_TOOL_CALL)
                                   << "== TOOL_CALL_UPDATE?" << (updateType == ACP::SESSION_UPDATE_TOOL_CALL_UPDATE);

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
    QString toolIdentifier = update[u"toolIdentifier"].toString();
    QString title = update[u"title"].toString();
    QString kind = update[u"kind"].toString();
    QString status = update[u"status"].toString();
    QJsonObject arguments = update[u"arguments"].toObject();

    // If toolIdentifier is empty, try to get it from _meta.tool_name
    if (toolIdentifier.isEmpty() && update.contains(u"_meta") && update[u"_meta"].isObject()) {
        QJsonObject meta = update[u"_meta"].toObject();
        toolIdentifier = meta[u"tool_name"].toString();
    }

    // Extract the command line from various possible locations
    QString command;

    // First, try to get from rawInput.command (common for shell tools)
    if (update.contains(u"rawInput") && update[u"rawInput"].isObject()) {
        QJsonObject rawInput = update[u"rawInput"].toObject();
        if (rawInput.contains(u"command") && rawInput[u"command"].isString()) {
            command = rawInput[u"command"].toString();
        }
    }

    // If not found in rawInput, try arguments
    if (command.isEmpty()) {
        if (arguments.contains(u"command") && arguments[u"command"].isString()) {
            command = arguments[u"command"].toString();
        } else if (arguments.contains(u"cmd") && arguments[u"cmd"].isString()) {
            command = arguments[u"cmd"].toString();
        } else if (arguments.contains(u"args") && arguments[u"args"].isArray()) {
            // Build command from args array
            QJsonArray args = arguments[u"args"].toArray();
            QStringList argList;
            for (const QJsonValue &argValue : args) {
                if (argValue.isString()) {
                    argList << argValue.toString();
                }
            }
            if (!argList.isEmpty()) {
                command = argList.join(QStringLiteral(" "));
            }
        }
    }

    // For shell commands, also check if the title contains the full command
    // (some agents put the full command in the title like "bash: ls /tmp")
    if (command.isEmpty() && !title.isEmpty()) {
        // Check if title contains a colon (indicating "tool: command" format)
        if (title.contains(u":")) {
            // Extract everything after the first colon + space
            int colonPos = title.indexOf(u":");
            if (colonPos >= 0 && colonPos < title.length() - 1) {
                command = title.mid(colonPos + 1).trimmed();
                // Use the tool name from before the colon as the title
                title = title.left(colonPos).trimmed();
            }
        }
        // Don't use title as command if there's no colon - the title is the tool name
    }

    // Store the tool call information for later permission lookup
    // Store the full update object which should contain all tool call details
    if (!toolCallId.isEmpty()) {
        // Only store if we don't have this toolCallId yet, or if this message has command info
        bool hasCommandInfo = !command.isEmpty() || (!title.isEmpty() && title.contains(u":"));
        if (!m_toolCalls.contains(toolCallId) || hasCommandInfo) {
            m_toolCalls[toolCallId] = update;
        }
    }

    ACPChatMessageWidget *msgWidget = new ACPChatMessageWidget(ACPChatMessageWidget::MessageType::ToolCall, m_chatDisplayContainer);
    msgWidget->setTimestamp(QDateTime::currentDateTime());
    msgWidget->setSender(i18n("Agent"));
    msgWidget->setToolCallInfo(toolCallId, title, kind, status, command);

    addMessageWidget(msgWidget);
}

void ACPClientChatWidget::handleToolCallStatusUpdate(const QJsonObject &update)
{
    QString toolCallId = update[u"toolCallId"].toString();
    QString status = update[u"status"].toString();
    QString contentText;
    QString title = update[u"title"].toString();

    // Extract command from various possible locations
    QString command;
    if (update.contains(u"rawInput") && update[u"rawInput"].isObject()) {
        QJsonObject rawInput = update[u"rawInput"].toObject();
        if (rawInput.contains(u"command") && rawInput[u"command"].isString()) {
            command = rawInput[u"command"].toString();
        }
    }

    // Also check if title contains the command (e.g., "bash: ls /tmp")
    if (command.isEmpty() && !title.isEmpty() && title.contains(u":")) {
        int colonPos = title.indexOf(u":");
        if (colonPos >= 0 && colonPos < title.length() - 1) {
            command = title.mid(colonPos + 1).trimmed();
        }
    }

    // Store/update the tool call information for later permission lookup
    if (!toolCallId.isEmpty()) {
        // Only update storage if this message has command information
        // Check if rawInput actually contains a command
        bool hasRawInputCommand = false;
        if (update.contains(u"rawInput") && update[u"rawInput"].isObject()) {
            QJsonObject rawInput = update[u"rawInput"].toObject();
            hasRawInputCommand = rawInput.contains(u"command");
        }

        bool hasCommandInfo = !command.isEmpty() || hasRawInputCommand || (!title.isEmpty() && title.contains(u":"));

        if (hasCommandInfo || !m_toolCalls.contains(toolCallId)) {
            m_toolCalls[toolCallId] = update;
        }
    }

    // If we have a command and a toolCallId, try to update the existing ToolCall widget
    if (!command.isEmpty() && !toolCallId.isEmpty()) {
        for (ACPChatMessageWidget *widget : m_messageWidgets) {
            if (widget->type() == ACPChatMessageWidget::MessageType::ToolCall && widget->toolCallId() == toolCallId && widget->toolCommand().isEmpty()) {
                // Found the ToolCall widget without a command - update it
                widget->setToolCallCommand(command);
                break;
            }
        }
    }

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

    // Update status bar with usage info
    QString statusText;
    if (used > 0 && size > 0) {
        statusText = i18n("Tokens: %1 / %2", used, size);
        if (cost > 0.0 && !currency.isEmpty()) {
            statusText += QStringLiteral(" | Cost: %1 %2").arg(cost, 0, 'f', 4).arg(currency);
        }
    }
    updateStatus(statusText);
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

    // Also update action states based on server initialization
    updateActionStates();
}

void ACPClientChatWidget::updateActionStates()
{
    // Check if there's an active, initialized server via the server manager
    bool hasActiveServer = m_serverManager && m_serverManager->activeServer();

    // Enable/disable actions based on server availability and session state
    m_ui->newSessionButton->setEnabled(hasActiveServer);
    m_ui->sendButton->setEnabled(hasActiveServer && !m_sessionId.isEmpty());
    m_ui->endSessionButton->setEnabled(hasActiveServer && !m_sessionId.isEmpty());
    m_ui->messageInput->setEnabled(hasActiveServer);
}

bool ACPClientChatWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_chatDisplayContainer && event->type() == QEvent::ContextMenu) {
        QContextMenuEvent *contextEvent = static_cast<QContextMenuEvent *>(event);

        QMenu *menu = new QMenu(m_chatDisplayContainer);
        QAction *copyAction = menu->addAction(i18n("Copy All"));
        copyAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
        connect(copyAction, &QAction::triggered, this, &ACPClientChatWidget::copyChatText);

        menu->exec(contextEvent->globalPos());
        delete menu;
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

#include "moc_acpclientchatwidget.cpp"
