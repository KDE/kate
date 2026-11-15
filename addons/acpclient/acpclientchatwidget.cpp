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

#include <QAbstractItemView>
#include <QClipboard>
#include <QComboBox>
#include <QCompleter>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLayout>
#include <QListWidgetItem>
#include <QMenu>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTextCursor>
#include <QTimer>

#include <utility>

#include "ui_acpclientchat.h"

/**
 * @brief Drop the cached layout measurements of a widget and its children
 *
 * Changing the text of a label invalidates the layout of its immediate parent
 * only. The box layouts further up keep the height they cached for a given
 * width until the posted layout request is delivered, which happens after
 * control returns to the event loop. Measuring a message right after it
 * changed therefore yields its previous height, and the message gets clipped.
 */
static void invalidateLayouts(QWidget *widget)
{
    const QList<QWidget *> children = widget->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *child : children) {
        invalidateLayouts(child);
    }

    if (QLayout *layout = widget->layout()) {
        layout->invalidate();
        layout->activate();
    }
}

ACPClientChatWidget::ACPClientChatWidget(ACPClientPlugin *plugin, KTextEditor::MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent)
    , m_plugin(plugin)
    , m_mainWindow(mainWindow)
    , m_ui(new Ui::ACPChatWidget)
{
    qCDebug(ACPCLIENT) << "ACPClientChatWidget created";

    m_ui->setupUi(this);

    // Complete slash commands while the user types in the message input
    m_commandCompletionModel = new QStringListModel(this);
    m_commandCompleter = new QCompleter(m_commandCompletionModel, this);
    m_commandCompleter->setCaseSensitivity(Qt::CaseSensitive);
    m_commandCompleter->setCompletionMode(QCompleter::PopupCompletion);
    m_commandCompleter->setWidget(m_ui->messageInput);
    connect(m_commandCompleter, QOverload<const QString &>::of(&QCompleter::activated), this, &ACPClientChatWidget::insertCommand);

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
    connect(m_ui->commandsButton, &QToolButton::clicked, this, &ACPClientChatWidget::showCommandsMenu);
    connect(m_ui->messageInput, &QTextEdit::textChanged, this, &ACPClientChatWidget::updateCommandCompletion);
    connect(m_ui->messageInput, &QTextEdit::cursorPositionChanged, this, &ACPClientChatWidget::updateCommandCompletion);

    // Apply session mode and model changes selected by the user
    connect(m_ui->modeCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        if (index < 0 || !m_serverManager || m_sessionId.isEmpty()) {
            return;
        }

        const QString modeId = m_ui->modeCombo->itemData(index).toString();
        if (modeId.isEmpty()) {
            return;
        }

        if (!m_modeConfigId.isEmpty()) {
            m_serverManager->setSessionConfigOption(m_sessionId, m_modeConfigId, modeId);
        } else {
            m_serverManager->setSessionMode(m_sessionId, modeId);
        }
    });

    connect(m_ui->modelCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        if (index < 0 || !m_serverManager || m_sessionId.isEmpty()) {
            return;
        }

        const QString modelId = m_ui->modelCombo->itemData(index).toString();
        if (modelId.isEmpty()) {
            return;
        }

        if (!m_modelConfigId.isEmpty()) {
            m_serverManager->setSessionConfigOption(m_sessionId, m_modelConfigId, modelId);
        } else {
            m_serverManager->setSessionModel(m_sessionId, modelId);
        }
    });

    // Set up QTextEdit for message input
    m_ui->messageInput->setPlaceholderText(i18n("Type your message here... (Ctrl+Enter to send, Shift+Enter for new line)"));
    m_ui->messageInput->setTabChangesFocus(true);
    m_ui->messageInput->setAcceptRichText(false);

    // Set up model with message history (for potential future use)
    m_historyModel = new QStringListModel(m_messageHistory, this);

    // Connect to textChanged signal for send button enablement
    connect(m_ui->messageInput, &QTextEdit::textChanged, this, &ACPClientChatWidget::onInputTextChanged);

    // Get the list widget from UI
    m_chatListWidget = m_ui->chatListWidget;
    m_chatListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_chatListWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_chatListWidget->setUniformItemSizes(false);

    // Scroll smoothly: the default per-item scrolling jumps a whole message
    // per wheel notch, which is unusable for long messages.
    m_chatListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_chatListWidget->verticalScrollBar()->setSingleStep(m_chatListWidget->fontMetrics().height());

    // Only follow new messages while the view is at the bottom. Appending
    // content does not change the scrollbar value, so this keeps the state the
    // user left the view in, and programmatic scrolling re-arms it.
    connect(m_chatListWidget->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        QScrollBar *scrollBar = m_chatListWidget->verticalScrollBar();
        m_followChatBottom = value >= scrollBar->maximum() - scrollBar->singleStep();
    });

    // Install event filter on chat list widget for context menu
    m_chatListWidget->installEventFilter(this);

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

    // Disconnect any existing sessionCreated connection to avoid duplicates
    disconnect(m_serverManager, &ACPClientServerManager::sessionCreated, this, nullptr);

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
        // Drop configuration and command state from the previous session
        clearSessionConfigUi();
        clearAvailableCommands();
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
        new ACPChatMessageWidget(isUser ? ACPChatMessageWidget::MessageType::User : ACPChatMessageWidget::MessageType::Agent, m_chatListWidget);

    msgWidget->setTimestamp(QDateTime::currentDateTime());
    msgWidget->setSender(isUser ? i18n("You") : sender);
    msgWidget->setContent(message);

    addMessageWidget(msgWidget);
}

void ACPClientChatWidget::addMessageWidget(ACPChatMessageWidget *widget)
{
    if (!widget || !m_chatListWidget) {
        return;
    }

    // Create a container widget to hold the message widget
    // This ensures proper size calculation
    QWidget *container = new QWidget(m_chatListWidget);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);
    containerLayout->addWidget(widget);

    // Create a list widget item
    QListWidgetItem *item = new QListWidgetItem(m_chatListWidget);

    // Add the container to the list
    m_chatListWidget->addItem(item);
    m_chatListWidget->setItemWidget(item, container);

    // Store the mappings
    m_widgetToContainerMap[widget] = container;
    m_containerToItemMap[container] = item;

    widget->show();
    m_messageWidgets.append(widget);

    // Re-measure the item when the message changes height, e.g. when the user
    // expands a collapsed tool call input or output
    connect(widget, &ACPChatMessageWidget::contentSizeChanged, this, [this, widget]() {
        updateWidgetSizeHint(widget);
    });

    // Measure once the widget has been laid out by the event loop
    QTimer::singleShot(0, this, [this, item, container]() {
        updateItemSizeHint(item, container);
        scrollToBottom();
    });
}

void ACPClientChatWidget::clearMessages()
{
    // Delete all message widgets and their containers
    for (ACPChatMessageWidget *widget : std::as_const(m_messageWidgets)) {
        QWidget *container = m_widgetToContainerMap.value(widget);
        if (container) {
            container->deleteLater();
        }
        widget->deleteLater();
    }
    m_messageWidgets.clear();
    m_widgetToContainerMap.clear();
    m_containerToItemMap.clear();

    // Clear the list widget
    if (m_chatListWidget) {
        m_chatListWidget->clear();
    }
}

void ACPClientChatWidget::updateWidgetSizeHint(ACPChatMessageWidget *widget)
{
    if (!widget || !m_chatListWidget) {
        return;
    }

    QWidget *container = m_widgetToContainerMap.value(widget);
    if (container) {
        auto itemIt = m_containerToItemMap.find(container);
        if (itemIt != m_containerToItemMap.end()) {
            QListWidgetItem *item = itemIt.value();
            updateItemSizeHint(item, container);
        }
    }
}

void ACPClientChatWidget::scrollToBottom()
{
    if (!m_chatListWidget) {
        return;
    }

    // Do not drag the view along while the user is reading further up
    if (!m_followChatBottom) {
        return;
    }

    // Use a small delay to ensure layout is complete
    QTimer::singleShot(0, this, [this]() {
        if (m_followChatBottom) {
            m_chatListWidget->scrollToBottom();
        }
    });
}

void ACPClientChatWidget::updateAllItemSizeHints()
{
    if (!m_chatListWidget) {
        return;
    }

    // Update all item size hints based on the current viewport width
    int availableWidth = m_chatListWidget->viewport()->width();
    if (availableWidth <= 0) {
        return;
    }

    for (int i = 0; i < m_chatListWidget->count(); ++i) {
        QListWidgetItem *item = m_chatListWidget->item(i);
        if (item) {
            QWidget *widget = m_chatListWidget->itemWidget(item);
            if (widget) {
                // Find the container for this item
                auto it = m_containerToItemMap.begin();
                for (; it != m_containerToItemMap.end(); ++it) {
                    if (it.key() == widget) {
                        updateItemSizeHint(item, it.key());
                        break;
                    }
                }
            }
        }
    }

    // After updating all sizes, ensure we're still scrolled to the bottom
    scrollToBottom();
}

void ACPClientChatWidget::updateItemSizeHint(QListWidgetItem *item, QWidget *container)
{
    if (!item || !container || !m_chatListWidget) {
        return;
    }

    container->ensurePolished();

    const int availableWidth = m_chatListWidget->viewport()->width();
    if (availableWidth <= 0) {
        // Not laid out yet, the natural size is the best guess available
        container->adjustSize();
        item->setSizeHint(container->sizeHint());
        return;
    }

    // Lay the message out at the width it is displayed with, otherwise word
    // wrapped text reports the height of a single long line
    if (container->width() != availableWidth) {
        container->resize(availableWidth, container->height());
    }

    // Discard heights cached for the previous content
    invalidateLayouts(container);

    // Ask for the height the message needs at this width, rather than the
    // height it would like at its preferred width
    int height = container->heightForWidth(availableWidth);
    if (height <= 0) {
        container->adjustSize();
        height = container->sizeHint().height();
    }

    item->setSizeHint(QSize(availableWidth, height));
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
    for (ACPChatMessageWidget *widget : std::as_const(m_messageWidgets)) {
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

void ACPClientChatWidget::onInputTextChanged()
{
    // Enable/disable send button based on whether there's text
    if (m_ui->sendButton) {
        m_ui->sendButton->setEnabled(!m_ui->messageInput->toPlainText().trimmed().isEmpty());
    }
}

void ACPClientChatWidget::onPermissionRequested(const QJsonValue &requestId, const QJsonObject &toolCall, const QJsonArray &options)
{
    // Log the request ID (handle both integer and string)
    QString requestIdStr;
    if (requestId.isDouble()) {
        requestIdStr = QString::number(requestId.toInteger());
    } else if (requestId.isString()) {
        requestIdStr = requestId.toString();
    } else {
        requestIdStr = QStringLiteral("invalid");
    }
    qCDebug(ACPCLIENT) << "Permission requested for requestId:" << requestIdStr << "with" << options.size() << "options";

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
        Q_EMIT permissionResponse(requestId, ACP::permissionKindRejectOnce());
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
                    if (optKind == ACP::permissionKindAllowAlways()) {
                        optionId = option[u"optionId"].toString();
                        break;
                    } else if (optKind == ACP::permissionKindAllowOnce() && optionId.isEmpty()) {
                        optionId = option[u"optionId"].toString();
                    }
                }
            }
            if (optionId.isEmpty()) {
                // Fallback to allow_once
                optionId = ACP::permissionKindAllowOnce();
            }
        } else { // DenyAll
            // Find a reject option, prefer reject_always then reject_once
            for (const QJsonValue &opt : options) {
                if (opt.isObject()) {
                    QJsonObject option = opt.toObject();
                    QString optKind = option[u"kind"].toString();
                    if (optKind == ACP::permissionKindRejectAlways()) {
                        optionId = option[u"optionId"].toString();
                        break;
                    } else if (optKind == ACP::permissionKindRejectOnce() && optionId.isEmpty()) {
                        optionId = option[u"optionId"].toString();
                    }
                }
            }
            if (optionId.isEmpty()) {
                // Fallback to reject_once
                optionId = ACP::permissionKindRejectOnce();
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
        const QJsonArray locations = fullToolCall[u"locations"].toArray();
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
    ACPChatMessageWidget *permissionWidget = new ACPChatMessageWidget(ACPChatMessageWidget::MessageType::PermissionRequest, m_chatListWidget);
    permissionWidget->setTimestamp(QDateTime::currentDateTime());
    permissionWidget->setSender(i18n("ACP Agent"));

    // Use the new method to support all permission options
    // Pass the tool name and command separately for better display
    permissionWidget->setPermissionRequestWithOptions(requestId, title, toolDisplayName, argumentsStr, permissionOptions);

    // Connect the widget's signal to our handler
    connect(permissionWidget, &ACPChatMessageWidget::permissionResponse, this, [this](const QJsonValue &reqId, const QString &optionId) {
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
            connect(this, &ACPClientChatWidget::permissionResponse, this, [server](const QJsonValue &requestId, const QString &optionId) {
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

    // Connect to session configuration signals
    connect(m_serverManager, &ACPClientServerManager::sessionModesChanged, this, [this](const QString &sessionId, const ACP::SessionModeState &modes) {
        if (sessionId != m_sessionId || m_hasConfigOptions) {
            return;
        }
        updateModeCombo(modes);
    });

    connect(m_serverManager, &ACPClientServerManager::sessionModelsChanged, this, [this](const QString &sessionId, const ACP::SessionModelState &models) {
        if (sessionId != m_sessionId || m_hasConfigOptions) {
            return;
        }
        updateModelCombo(models);
    });

    connect(m_serverManager,
            &ACPClientServerManager::sessionConfigOptionsChanged,
            this,
            [this](const QString &sessionId, const QList<ACP::SessionConfigOption> &configOptions) {
                if (sessionId != m_sessionId) {
                    return;
                }
                m_hasConfigOptions = true;
                updateConfigOptionCombos(configOptions);
            });
}

void ACPClientChatWidget::sendMessage()
{
    QString message = m_ui->messageInput->toPlainText().trimmed();
    if (message.isEmpty()) {
        return;
    }

    // Add to history
    if (!m_messageHistory.contains(message)) {
        m_messageHistory.prepend(message);
        // Update the history model
        if (m_historyModel) {
            m_historyModel->setStringList(m_messageHistory);
        }
    }

    // Display user message
    appendMessage(i18n("You"), message, true);
    m_ui->messageInput->clear();

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
    if (obj.contains(u"method") && obj[u"method"].toString() == ACP::notificationSessionUpdate()) {
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

                qCDebug(ACPCLIENT) << "Session update type:" << updateType << "== TOOL_CALL?" << (updateType == ACP::sessionUpdateToolCall())
                                   << "== TOOL_CALL_UPDATE?" << (updateType == ACP::sessionUpdateToolCallUpdate());

                // Handle different update types
                if (updateType == ACP::sessionUpdateAgentMessageChunk()) {
                    handleAgentMessageChunk(update);
                } else if (updateType == ACP::sessionUpdatePlan()) {
                    handlePlanUpdate(update);
                } else if (updateType == ACP::sessionUpdateToolCall()) {
                    handleToolCallUpdate(update);
                } else if (updateType == ACP::sessionUpdateToolCallUpdate()) {
                    handleToolCallStatusUpdate(update);
                } else if (updateType == ACP::sessionUpdateUsageUpdate()) {
                    handleUsageUpdate(update);
                } else if (updateType == ACP::sessionUpdateCurrentMode()) {
                    handleCurrentModeUpdate(update);
                } else if (updateType == ACP::sessionUpdateConfigOption()) {
                    handleConfigOptionUpdate(update);
                } else if (updateType == ACP::sessionUpdateAvailableCommandsUpdate() || updateType == ACP::sessionUpdateAvailableCommands()) {
                    handleAvailableCommandsUpdate(update);
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
    if (obj.contains(u"method") && obj[u"method"].toString() == ACP::notificationProgress()) {
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
            if (messageId != m_lastMessageId && !messageId.isEmpty()) {
                // New message
                m_lastMessageId = messageId;
                ACPChatMessageWidget *msgWidget = new ACPChatMessageWidget(ACPChatMessageWidget::MessageType::Agent, m_chatListWidget);
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
                        lastWidget->setContent(existingContent + text);
                        // Update the item size hint
                        QTimer::singleShot(10, this, [this, lastWidget]() {
                            updateWidgetSizeHint(lastWidget);
                            scrollToBottom();
                        });
                        return;
                    }
                }
                // Fallback: create new message
                ACPChatMessageWidget *msgWidget = new ACPChatMessageWidget(ACPChatMessageWidget::MessageType::Agent, m_chatListWidget);
                msgWidget->setTimestamp(QDateTime::currentDateTime());
                msgWidget->setSender(i18n("Agent"));
                msgWidget->setContent(text);
                addMessageWidget(msgWidget);
            } else {
                // No message ID - try to update the last agent message
                if (!m_messageWidgets.isEmpty()) {
                    // Find the last Agent message widget
                    ACPChatMessageWidget *lastAgentWidget = nullptr;
                    for (auto it = m_messageWidgets.rbegin(); it != m_messageWidgets.rend(); ++it) {
                        if ((*it)->type() == ACPChatMessageWidget::MessageType::Agent) {
                            lastAgentWidget = *it;
                            break;
                        }
                    }

                    if (lastAgentWidget) {
                        // Append to the last agent message
                        QString existingContent = lastAgentWidget->content();
                        lastAgentWidget->setContent(existingContent + text);
                        // Update the item size hint and scroll
                        QTimer::singleShot(10, this, [this, lastAgentWidget]() {
                            updateWidgetSizeHint(lastAgentWidget);
                            scrollToBottom();
                        });
                        return;
                    }
                }

                // Fallback: create new message if no agent message exists
                ACPChatMessageWidget *msgWidget = new ACPChatMessageWidget(ACPChatMessageWidget::MessageType::Agent, m_chatListWidget);
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
        const QJsonArray entries = update[u"entries"].toArray();

        ACPChatMessageWidget *msgWidget = new ACPChatMessageWidget(ACPChatMessageWidget::MessageType::Plan, m_chatListWidget);
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
            const QJsonArray args = arguments[u"args"].toArray();
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

    ACPChatMessageWidget *msgWidget = new ACPChatMessageWidget(ACPChatMessageWidget::MessageType::ToolCall, m_chatListWidget);
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
        for (ACPChatMessageWidget *widget : std::as_const(m_messageWidgets)) {
            if (widget->type() == ACPChatMessageWidget::MessageType::ToolCall && widget->toolCallId() == toolCallId && widget->toolCommand().isEmpty()) {
                // Found the ToolCall widget without a command - update it
                widget->setToolCallCommand(command);
                break;
            }
        }
    }

    // Check for content
    if (update.contains(u"content") && update[u"content"].isArray()) {
        const QJsonArray contentArray = update[u"content"].toArray();
        for (const QJsonValue &contentValue : contentArray) {
            if (contentValue.isObject()) {
                const QJsonObject content = contentValue.toObject();
                if (content.contains(u"content") && content[u"content"].isObject()) {
                    const QJsonObject innerContent = content[u"content"].toObject();
                    if (innerContent.contains(u"type") && innerContent[u"type"].toString() == QStringLiteral("text")) {
                        contentText = innerContent[u"text"].toString();
                        break;
                    }
                }
            }
        }
    }

    // Find the existing ToolCall widget for this toolCallId and update it
    if (!toolCallId.isEmpty()) {
        for (ACPChatMessageWidget *widget : std::as_const(m_messageWidgets)) {
            if (widget->type() == ACPChatMessageWidget::MessageType::ToolCall && widget->toolCallId() == toolCallId) {
                // Found the ToolCall widget - update its status and content
                widget->setToolCallStatus(toolCallId, status, contentText);
                // Update the item size hint
                QTimer::singleShot(10, this, [this, widget]() {
                    updateWidgetSizeHint(widget);
                    scrollToBottom();
                });
                break;
            }
        }
    }
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

void ACPClientChatWidget::updateModeCombo(const ACP::SessionModeState &modes)
{
    m_modeConfigId.clear();

    const QSignalBlocker blocker(m_ui->modeCombo);
    m_ui->modeCombo->clear();

    for (const ACP::SessionMode &mode : modes.availableModes) {
        m_ui->modeCombo->addItem(mode.name, mode.id);
        if (!mode.description.isEmpty()) {
            m_ui->modeCombo->setItemData(m_ui->modeCombo->count() - 1, mode.description, Qt::ToolTipRole);
        }
    }

    const int currentIndex = m_ui->modeCombo->findData(modes.currentModeId);
    if (currentIndex >= 0) {
        m_ui->modeCombo->setCurrentIndex(currentIndex);
    }

    const bool visible = m_ui->modeCombo->count() > 0;
    m_ui->modeLabel->setVisible(visible);
    m_ui->modeCombo->setVisible(visible);

    updateActionStates();
}

void ACPClientChatWidget::updateModelCombo(const ACP::SessionModelState &models)
{
    m_modelConfigId.clear();

    const QSignalBlocker blocker(m_ui->modelCombo);
    m_ui->modelCombo->clear();

    for (const ACP::SessionModel &model : models.availableModels) {
        m_ui->modelCombo->addItem(model.name, model.modelId);
        if (!model.description.isEmpty()) {
            m_ui->modelCombo->setItemData(m_ui->modelCombo->count() - 1, model.description, Qt::ToolTipRole);
        }
    }

    const int currentIndex = m_ui->modelCombo->findData(models.currentModelId);
    if (currentIndex >= 0) {
        m_ui->modelCombo->setCurrentIndex(currentIndex);
    }

    const bool visible = m_ui->modelCombo->count() > 0;
    m_ui->modelLabel->setVisible(visible);
    m_ui->modelCombo->setVisible(visible);

    updateActionStates();
}

void ACPClientChatWidget::updateConfigOptionCombos(const QList<ACP::SessionConfigOption> &options)
{
    const ACP::SessionConfigOption *modeOption = nullptr;
    const ACP::SessionConfigOption *modelOption = nullptr;

    for (const ACP::SessionConfigOption &option : options) {
        if (!modeOption && option.category == QStringLiteral("mode")) {
            modeOption = &option;
        }
        if (!modelOption
            && (option.category == QStringLiteral("model") || option.id == QStringLiteral("model")
                || option.name.compare(QStringLiteral("model"), Qt::CaseInsensitive) == 0)) {
            modelOption = &option;
        }
    }

    m_modeConfigId.clear();
    {
        const QSignalBlocker blocker(m_ui->modeCombo);
        m_ui->modeCombo->clear();

        if (modeOption) {
            m_modeConfigId = modeOption->id;
            for (const ACP::SessionConfigSelectOption &value : modeOption->options) {
                m_ui->modeCombo->addItem(value.name, value.value);
                if (!value.description.isEmpty()) {
                    m_ui->modeCombo->setItemData(m_ui->modeCombo->count() - 1, value.description, Qt::ToolTipRole);
                }
            }
            const int currentIndex = m_ui->modeCombo->findData(modeOption->currentValue);
            if (currentIndex >= 0) {
                m_ui->modeCombo->setCurrentIndex(currentIndex);
            }
        }

        const bool visible = m_ui->modeCombo->count() > 0;
        m_ui->modeLabel->setVisible(visible);
        m_ui->modeCombo->setVisible(visible);
    }

    m_modelConfigId.clear();
    {
        const QSignalBlocker blocker(m_ui->modelCombo);
        m_ui->modelCombo->clear();

        if (modelOption) {
            m_modelConfigId = modelOption->id;
            for (const ACP::SessionConfigSelectOption &value : modelOption->options) {
                m_ui->modelCombo->addItem(value.name, value.value);
                if (!value.description.isEmpty()) {
                    m_ui->modelCombo->setItemData(m_ui->modelCombo->count() - 1, value.description, Qt::ToolTipRole);
                }
            }
            const int currentIndex = m_ui->modelCombo->findData(modelOption->currentValue);
            if (currentIndex >= 0) {
                m_ui->modelCombo->setCurrentIndex(currentIndex);
            }
        }

        const bool visible = m_ui->modelCombo->count() > 0;
        m_ui->modelLabel->setVisible(visible);
        m_ui->modelCombo->setVisible(visible);
    }

    updateActionStates();
}

void ACPClientChatWidget::clearSessionConfigUi()
{
    m_hasConfigOptions = false;
    m_modeConfigId.clear();
    m_modelConfigId.clear();

    {
        const QSignalBlocker blocker(m_ui->modeCombo);
        m_ui->modeCombo->clear();
        m_ui->modeCombo->setVisible(false);
        m_ui->modeLabel->setVisible(false);
    }

    {
        const QSignalBlocker blocker(m_ui->modelCombo);
        m_ui->modelCombo->clear();
        m_ui->modelCombo->setVisible(false);
        m_ui->modelLabel->setVisible(false);
    }
}

void ACPClientChatWidget::handleCurrentModeUpdate(const QJsonObject &update)
{
    const QString modeId = update[u"currentModeId"].toString();
    if (modeId.isEmpty()) {
        return;
    }

    const QSignalBlocker blocker(m_ui->modeCombo);
    const int index = m_ui->modeCombo->findData(modeId);
    if (index >= 0) {
        m_ui->modeCombo->setCurrentIndex(index);
    }
}

void ACPClientChatWidget::handleConfigOptionUpdate(const QJsonObject &update)
{
    if (!update.contains(u"configOptions") || !update[u"configOptions"].isArray()) {
        return;
    }

    QList<ACP::SessionConfigOption> configOptions;
    if (ACP::ACPProtocol::parseSessionConfigOptions(update[u"configOptions"].toArray(), configOptions)) {
        m_hasConfigOptions = true;
        updateConfigOptionCombos(configOptions);
    }
}

void ACPClientChatWidget::handleAvailableCommandsUpdate(const QJsonObject &update)
{
    QList<ACP::AvailableCommand> commands;
    if (ACP::ACPProtocol::parseAvailableCommands(update[u"availableCommands"].toArray(), commands)) {
        updateAvailableCommands(commands);
    }
}

void ACPClientChatWidget::updateAvailableCommands(const QList<ACP::AvailableCommand> &commands)
{
    m_availableCommands = commands;

    QStringList commandNames;
    commandNames.reserve(m_availableCommands.size());
    for (const ACP::AvailableCommand &command : std::as_const(m_availableCommands)) {
        commandNames.append(command.name);
    }
    m_commandCompletionModel->setStringList(commandNames);

    m_ui->commandsButton->setVisible(!m_availableCommands.isEmpty());
    updateActionStates();
}

void ACPClientChatWidget::clearAvailableCommands()
{
    m_availableCommands.clear();
    m_commandCompletionModel->setStringList(QStringList());
    m_commandCompleter->popup()->hide();
    m_ui->commandsButton->setVisible(false);
}

void ACPClientChatWidget::showCommandsMenu()
{
    if (m_availableCommands.isEmpty()) {
        return;
    }

    QMenu menu(this);
    menu.setToolTipsVisible(true);

    for (const ACP::AvailableCommand &command : std::as_const(m_availableCommands)) {
        QAction *action = menu.addAction(QStringLiteral("/") + command.name);

        QString toolTip = command.description;
        if (!command.inputHint.isEmpty()) {
            if (!toolTip.isEmpty()) {
                toolTip += QLatin1Char('\n');
            }
            toolTip += i18n("Input: %1", command.inputHint);
        }
        action->setToolTip(toolTip);

        connect(action, &QAction::triggered, this, [this, name = command.name] {
            insertCommand(name);
        });
    }

    const QPoint topLeft = m_ui->commandsButton->mapToGlobal(QPoint(0, 0));
    menu.exec(QPoint(topLeft.x(), topLeft.y() - menu.sizeHint().height()));
}

void ACPClientChatWidget::updateCommandCompletion()
{
    if (!m_commandCompleter) {
        return;
    }

    if (m_availableCommands.isEmpty() || m_sessionId.isEmpty()) {
        m_commandCompleter->popup()->hide();
        return;
    }

    const QTextEdit *input = m_ui->messageInput;
    const QString text = input->toPlainText();
    const int cursorPosition = input->textCursor().position();

    // Commands are only recognized at the start of the prompt.
    if (text.isEmpty() || text.at(0) != QLatin1Char('/')) {
        m_commandCompleter->popup()->hide();
        return;
    }

    // Stop suggesting once the cursor is past the command token.
    int tokenEnd = text.size();
    for (int i = 1; i < text.size(); ++i) {
        if (text.at(i).isSpace()) {
            tokenEnd = i;
            break;
        }
    }
    if (cursorPosition > tokenEnd) {
        m_commandCompleter->popup()->hide();
        return;
    }

    const QString prefix = cursorPosition > 1 ? text.mid(1, cursorPosition - 1) : QString();
    if (m_commandCompleter->popup()->isVisible() && m_commandCompleter->completionPrefix() == prefix) {
        return;
    }

    m_commandCompleter->setCompletionPrefix(prefix);
    if (m_commandCompleter->completionCount() == 0) {
        m_commandCompleter->popup()->hide();
        return;
    }

    QRect popupRect = input->cursorRect();
    popupRect.translate(input->viewport()->mapTo(input, QPoint(0, 0)));
    popupRect.setWidth(qMax(popupRect.width(), 200));
    m_commandCompleter->complete(popupRect);
}

void ACPClientChatWidget::insertCommand(const QString &name)
{
    const QString text = m_ui->messageInput->toPlainText();

    int tokenStart = 0;
    while (tokenStart < text.size() && text.at(tokenStart).isSpace()) {
        ++tokenStart;
    }

    int tokenEnd = tokenStart;
    while (tokenEnd < text.size() && !text.at(tokenEnd).isSpace()) {
        ++tokenEnd;
    }

    // Replace an existing command token, otherwise insert at the cursor.
    if (tokenStart < text.size() && text.at(tokenStart) == QLatin1Char('/')) {
        QTextCursor cursor = m_ui->messageInput->textCursor();
        cursor.setPosition(tokenStart);
        cursor.setPosition(tokenEnd, QTextCursor::KeepAnchor);
        cursor.insertText(QStringLiteral("/") + name + QLatin1Char(' '));
        m_ui->messageInput->setTextCursor(cursor);
    } else {
        QTextCursor cursor = m_ui->messageInput->textCursor();
        cursor.insertText(QStringLiteral("/") + name + QLatin1Char(' '));
        m_ui->messageInput->setTextCursor(cursor);
    }

    m_ui->messageInput->setFocus();
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

    const bool sessionActive = hasActiveServer && !m_sessionId.isEmpty();

    // Enable/disable actions based on server availability and session state
    m_ui->newSessionButton->setEnabled(hasActiveServer);
    m_ui->sendButton->setEnabled(sessionActive);
    m_ui->endSessionButton->setEnabled(sessionActive);
    m_ui->messageInput->setEnabled(sessionActive);
    m_ui->modeCombo->setEnabled(sessionActive && m_ui->modeCombo->count() > 0);
    m_ui->modelCombo->setEnabled(sessionActive && m_ui->modelCombo->count() > 0);
    m_ui->commandsButton->setEnabled(sessionActive && !m_availableCommands.isEmpty());
}

bool ACPClientChatWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_chatListWidget && event->type() == QEvent::ContextMenu) {
        QContextMenuEvent *contextEvent = static_cast<QContextMenuEvent *>(event);

        QMenu *menu = new QMenu(m_chatListWidget);
        QAction *copyAction = menu->addAction(i18n("Copy All"));
        copyAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
        connect(copyAction, &QAction::triggered, this, &ACPClientChatWidget::copyChatText);

        menu->exec(contextEvent->globalPos());
        delete menu;
        return true;
    }

    // Handle resize events on the list widget to update item sizes
    if (watched == m_chatListWidget && event->type() == QEvent::Resize) {
        // When the chat list is resized, we need to update all item sizes
        // to account for the new width and proper word wrapping
        updateAllItemSizeHints();
        return true;
    }

    // Handle resize events on the viewport as well
    if (watched == m_chatListWidget->viewport() && event->type() == QEvent::Resize) {
        updateAllItemSizeHints();
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void ACPClientChatWidget::keyPressEvent(QKeyEvent *event)
{
    // Handle Ctrl+Enter for sending message when message input has focus
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (event->modifiers() == Qt::ControlModifier) {
            // Ctrl+Enter: send message
            if (m_ui->messageInput->hasFocus()) {
                sendMessage();
                event->accept();
                return;
            }
        } else if (event->modifiers() == Qt::ShiftModifier) {
            // Shift+Enter: insert new line (handled by QTextEdit's default behavior)
            // Just need to make sure we don't interfere
        }
    }

    QWidget::keyPressEvent(event);
}

#include "moc_acpclientchatwidget.cpp"
