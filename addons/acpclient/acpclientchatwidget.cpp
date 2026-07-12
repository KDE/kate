/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#include "acpclientchatwidget.h"
#include "acpclient_debug.h"
#include "acpclientplugin.h"
#include "acpclientprotocol.h"
#include "acpclientserver.h"
#include "acpclientservermanager.h"

#include <KLocalizedString>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>

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

    // Set up chat display styling
    QTextDocument *doc = m_ui->chatDisplay->document();
    doc->setDefaultStyleSheet(
        QStringLiteral("body { background-color: #f0f0f0; }"
                       "p.user { color: #2c3e50; margin-left: 10px; }"
                       "p.agent { color: #27ae60; margin-left: 10px; }"
                       "span.timestamp { color: #7f8c8d; font-size: small; }"));

    // Enable rich text for styling
    m_ui->chatDisplay->setAcceptRichText(true);

    // Update UI state
    updateSessionState();
}

ACPClientChatWidget::~ACPClientChatWidget()
{
    qCDebug(ACPCLIENT) << "ACPClientChatWidget destroyed";
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
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        appendMessage(QStringLiteral("System"), i18n("No active ACP server. Please connect to an agent first."));
        return;
    }

    m_server = server;

    // Create a new session
    QString sessionId = m_serverManager->createSession();
    if (sessionId.isEmpty()) {
        appendMessage(QStringLiteral("System"), i18n("Failed to create new session"));
        return;
    }

    setSessionId(sessionId);

    // Connect to server messages
    connect(server, &ACPClientServer::messageReceived, this, &ACPClientChatWidget::onServerMessageReceived);
    connect(server, &ACPClientServer::disconnected, this, [this]() {
        appendMessage(QStringLiteral("System"), i18n("Server disconnected"));
        setSessionId(QString());
        updateSessionState();
    });

    appendMessage(QStringLiteral("System"), i18n("New ACP session started: %1", sessionId));
    updateSessionState();
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
    QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"));
    QString htmlMessage;

    if (isUser) {
        htmlMessage =
            QStringLiteral("<p class='user'><span class='timestamp'>[%1]</span> <strong>You:</strong> %2</p>").arg(timestamp, message.toHtmlEscaped());
    } else {
        htmlMessage = QStringLiteral("<p class='agent'><span class='timestamp'>[%1]</span> <strong>%2:</strong> %3</p>")
                          .arg(timestamp, sender.toHtmlEscaped(), message.toHtmlEscaped());
    }

    m_ui->chatDisplay->insertHtml(htmlMessage);
    m_ui->chatDisplay->ensureCursorVisible();

    // Auto-scroll to bottom
    QTextCursor cursor = m_ui->chatDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_ui->chatDisplay->setTextCursor(cursor);
}

void ACPClientChatWidget::clearChat()
{
    m_ui->chatDisplay->clear();
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
    if (!m_sessionId.isEmpty() && m_server && m_server->state() == ACPClientServer::ServerState::Initialized) {
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
    qCDebug(ACPCLIENT) << "Received server message:" << message.toJson();

    if (!message.isObject()) {
        return;
    }

    QJsonObject obj = message.object();

    // Handle session update notifications
    if (obj.contains(u"method") && obj[u"method"].toString() == ACP::NOTIFICATION_SESSION_UPDATE) {
        if (obj.contains(u"params") && obj[u"params"].isObject()) {
            QJsonObject params = obj[u"params"].toObject();
            QString sessionId = params[u"sessionId"].toString();
            QString status = params[u"status"].toString();
            QString msg = params[u"message"].toString();

            if (!msg.isEmpty() && sessionId == m_sessionId) {
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