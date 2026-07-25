/*
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpchatmessagewidget.h"

#include <KLocalizedString>

#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>

ACPChatMessageWidget::ACPChatMessageWidget(MessageType type, QWidget *parent)
    : QWidget(parent)
    , m_type(type)
    , m_timestamp(QDateTime::currentDateTime())
    , m_usedTokens(0)
    , m_sizeTokens(0)
    , m_cost(0.0)
{
    setupUI();
}

ACPChatMessageWidget::~ACPChatMessageWidget()
{
    // Widgets are deleted automatically by Qt's parent-child system
}

void ACPChatMessageWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 4, 8, 4);
    m_mainLayout->setSpacing(4);

    // Header widget with timestamp, sender, and type
    m_headerWidget = new QWidget(this);
    QHBoxLayout *headerLayout = new QHBoxLayout(m_headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    m_timestampLabel = new QLabel(this);
    m_timestampLabel->setObjectName(QStringLiteral("timestampLabel"));
    m_timestampLabel->setTextFormat(Qt::PlainText);
    headerLayout->addWidget(m_timestampLabel);

    m_senderLabel = new QLabel(this);
    m_senderLabel->setObjectName(QStringLiteral("senderLabel"));
    m_senderLabel->setTextFormat(Qt::PlainText);
    QFont boldFont = m_senderLabel->font();
    boldFont.setBold(true);
    m_senderLabel->setFont(boldFont);
    headerLayout->addWidget(m_senderLabel);

    m_typeLabel = new QLabel(this);
    m_typeLabel->setObjectName(QStringLiteral("typeLabel"));
    headerLayout->addWidget(m_typeLabel);

    headerLayout->addStretch();

    m_mainLayout->addWidget(m_headerWidget);

    // Content widget
    m_contentWidget = new QWidget(this);
    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(2);
    m_mainLayout->addWidget(m_contentWidget);

    // Set type-specific styling using palette colors
    applyTypeSpecificStyling();
    updateContentDisplay();
}

void ACPChatMessageWidget::setContent(const QString &content)
{
    m_content = content;
    updateContentDisplay();
}

void ACPChatMessageWidget::setSender(const QString &sender)
{
    m_sender = sender;
    m_senderLabel->setText(sender);
}

void ACPChatMessageWidget::setTimestamp(const QDateTime &timestamp)
{
    m_timestamp = timestamp;
    m_timestampLabel->setText(formatTimestamp(timestamp));
}

void ACPChatMessageWidget::setMessageId(const QString &messageId)
{
    m_messageId = messageId;
}

void ACPChatMessageWidget::addPlanEntry(const QString &content, const QString &priority, const QString &status)
{
    m_planEntries.append({content, priority, status});
    updateContentDisplay();
}

void ACPChatMessageWidget::setToolCallInfo(const QString &toolCallId, const QString &title, const QString &kind, const QString &status)
{
    m_toolCallId = toolCallId;
    m_toolTitle = title;
    m_toolKind = kind;
    m_toolStatus = status;
    updateContentDisplay();
}

void ACPChatMessageWidget::setToolCallStatus(const QString &toolCallId, const QString &status, const QString &content)
{
    m_toolCallId = toolCallId;
    m_toolStatus = status;
    m_toolContent = content;
    updateContentDisplay();
}

void ACPChatMessageWidget::setUsageInfo(qint64 used, qint64 size, double cost, const QString &currency)
{
    m_usedTokens = used;
    m_sizeTokens = size;
    m_cost = cost;
    m_currency = currency;
    updateContentDisplay();
}

void ACPChatMessageWidget::setPermissionRequest(qint64 requestId,
                                                const QString &title,
                                                const QString &command,
                                                const QString &allowOptionId,
                                                const QString &rejectOptionId)
{
    m_requestId = requestId;
    m_permissionTitle = title;
    m_permissionCommand = command;
    m_permissionAllowOptionId = allowOptionId;
    m_permissionRejectOptionId = rejectOptionId;
    updateContentDisplay();
}

ACPChatMessageWidget::MessageType ACPChatMessageWidget::type() const
{
    return m_type;
}

QString ACPChatMessageWidget::messageId() const
{
    return m_messageId;
}

QString ACPChatMessageWidget::content() const
{
    return m_content;
}

void ACPChatMessageWidget::applyTypeSpecificStyling()
{
    // Avoid stylesheets - use only QPalette and QFont
    setAutoFillBackground(true);
}

void ACPChatMessageWidget::updateContentDisplay()
{
    // Clear existing content
    QLayoutItem *child;
    while ((child = m_contentWidget->layout()->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }

    QVBoxLayout *contentLayout = qobject_cast<QVBoxLayout *>(m_contentWidget->layout());

    switch (m_type) {
    case MessageType::User:
    case MessageType::Agent:
    case MessageType::System: {
        QLabel *contentLabel = new QLabel(m_content, m_contentWidget);
        contentLabel->setWordWrap(true);
        contentLabel->setTextFormat(Qt::RichText);
        contentLabel->setOpenExternalLinks(true);
        contentLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        contentLayout->addWidget(contentLabel);
        break;
    }
    case MessageType::Plan: {
        for (const PlanEntry &entry : m_planEntries) {
            QWidget *entryWidget = new QWidget(m_contentWidget);
            QHBoxLayout *entryLayout = new QHBoxLayout(entryWidget);
            entryLayout->setContentsMargins(4, 2, 4, 2);
            entryLayout->setSpacing(4);

            QLabel *priorityLabel = new QLabel(entry.priority, entryWidget);
            QFont boldFont = priorityLabel->font();
            boldFont.setBold(true);
            priorityLabel->setFont(boldFont);
            // Set color based on priority using palette-appropriate colors
            QPalette priPal = priorityLabel->palette();
            if (entry.priority == QStringLiteral("high")) {
                priPal.setColor(QPalette::WindowText, priPal.color(QPalette::BrightText));
            } else if (entry.priority == QStringLiteral("medium")) {
                priPal.setColor(QPalette::WindowText, priPal.color(QPalette::Link));
            } else {
                priPal.setColor(QPalette::WindowText, priPal.color(QPalette::Text));
            }
            priorityLabel->setPalette(priPal);
            entryLayout->addWidget(priorityLabel);

            // Status
            if (!entry.status.isEmpty()) {
                QLabel *statusLabel = new QLabel(QStringLiteral("[%1]").arg(entry.status), entryWidget);
                QFont smallFont = statusLabel->font();
                smallFont.setPointSize(smallFont.pointSize() - 2);
                statusLabel->setFont(smallFont);
                entryLayout->addWidget(statusLabel);
            }

            QLabel *contentLabel = new QLabel(entry.content, entryWidget);
            contentLabel->setWordWrap(true);
            contentLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            entryLayout->addWidget(contentLabel, 1);

            contentLayout->addWidget(entryWidget);
        }
        break;
    }
    case MessageType::ToolCall: {
        QWidget *toolWidget = new QWidget(m_contentWidget);
        QHBoxLayout *toolLayout = new QHBoxLayout(toolWidget);
        toolLayout->setContentsMargins(4, 2, 4, 2);
        toolLayout->setSpacing(4);

        if (!m_toolTitle.isEmpty()) {
            QLabel *titleLabel = new QLabel(m_toolTitle, toolWidget);
            QFont boldFont = titleLabel->font();
            boldFont.setBold(true);
            titleLabel->setFont(boldFont);
            titleLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            toolLayout->addWidget(titleLabel);
        }

        if (!m_toolKind.isEmpty()) {
            QLabel *kindLabel = new QLabel(QStringLiteral("(%1)").arg(m_toolKind), toolWidget);
            QFont smallFont = kindLabel->font();
            smallFont.setPointSize(smallFont.pointSize() - 2);
            kindLabel->setFont(smallFont);
            kindLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            toolLayout->addWidget(kindLabel);
        }

        QLabel *statusLabel = new QLabel(m_toolStatus, toolWidget);
        QFont boldFont2 = statusLabel->font();
        boldFont2.setBold(true);
        statusLabel->setFont(boldFont2);
        // Set status color based on state using palette colors
        QPalette statusPal = statusLabel->palette();
        if (m_toolStatus == QStringLiteral("pending") || m_toolStatus == QStringLiteral("in_progress")) {
            statusPal.setColor(QPalette::WindowText, statusPal.color(QPalette::Link));
        } else if (m_toolStatus == QStringLiteral("completed")) {
            statusPal.setColor(QPalette::WindowText, statusPal.color(QPalette::LinkVisited));
        } else if (m_toolStatus == QStringLiteral("error") || m_toolStatus == QStringLiteral("failed")) {
            statusPal.setColor(QPalette::WindowText, statusPal.color(QPalette::BrightText));
        }
        statusLabel->setPalette(statusPal);
        statusLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        toolLayout->addWidget(statusLabel);

        if (!m_toolCallId.isEmpty()) {
            QLabel *idLabel = new QLabel(QStringLiteral("[ID: %1]").arg(m_toolCallId), toolWidget);
            QFont smallFont = idLabel->font();
            smallFont.setPointSize(smallFont.pointSize() - 2);
            idLabel->setFont(smallFont);
            idLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            toolLayout->addWidget(idLabel);
        }

        contentLayout->addWidget(toolWidget);
        break;
    }
    case MessageType::ToolCallUpdate: {
        QWidget *updateWidget = new QWidget(m_contentWidget);
        QHBoxLayout *updateLayout = new QHBoxLayout(updateWidget);
        updateLayout->setContentsMargins(4, 2, 4, 2);
        updateLayout->setSpacing(4);

        // Status color - using palette colors for dark/light mode support
        QPalette updatePal = updateWidget->palette();
        QPalette svPal = updateWidget->palette();

        if (m_toolStatus == QStringLiteral("pending") || m_toolStatus == QStringLiteral("in_progress")) {
            svPal.setColor(QPalette::WindowText, updatePal.color(QPalette::Link));
        } else if (m_toolStatus == QStringLiteral("completed")) {
            svPal.setColor(QPalette::WindowText, updatePal.color(QPalette::LinkVisited));
        } else {
            svPal.setColor(QPalette::WindowText, updatePal.color(QPalette::Text));
        }

        QLabel *statusLabel = new QLabel(i18n("Tool Update:"), updateWidget);
        QFont boldFont = statusLabel->font();
        boldFont.setBold(true);
        statusLabel->setFont(boldFont);
        statusLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        updateLayout->addWidget(statusLabel);

        QLabel *statusValueLabel = new QLabel(m_toolStatus, updateWidget);
        QFont boldFont2 = statusValueLabel->font();
        boldFont2.setBold(true);
        statusValueLabel->setFont(boldFont2);
        statusValueLabel->setPalette(svPal);
        statusValueLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        updateLayout->addWidget(statusValueLabel);

        if (!m_toolCallId.isEmpty()) {
            QLabel *idLabel = new QLabel(QStringLiteral("[ID: %1]").arg(m_toolCallId), updateWidget);
            QFont smallFont2 = idLabel->font();
            smallFont2.setPointSize(smallFont2.pointSize() - 2);
            idLabel->setFont(smallFont2);
            idLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            updateLayout->addWidget(idLabel);
        }

        if (!m_toolContent.isEmpty()) {
            QLabel *contentLabel = new QLabel(m_toolContent, updateWidget);
            contentLabel->setWordWrap(true);
            contentLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            updateLayout->addWidget(contentLabel, 1);
        }

        contentLayout->addWidget(updateWidget);
        break;
    }
    case MessageType::Usage: {
        QWidget *usageWidget = new QWidget(m_contentWidget);
        QHBoxLayout *usageLayout = new QHBoxLayout(usageWidget);
        usageLayout->setContentsMargins(4, 2, 4, 2);
        usageLayout->setSpacing(8);

        QLabel *usageLabel = new QLabel(i18n("Usage:"), usageWidget);
        QFont boldFont = usageLabel->font();
        boldFont.setBold(true);
        usageLabel->setFont(boldFont);
        usageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        usageLayout->addWidget(usageLabel);

        QLabel *tokensLabel = new QLabel(QStringLiteral("%1 / %2 tokens").arg(m_usedTokens).arg(m_sizeTokens), usageWidget);
        tokensLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        usageLayout->addWidget(tokensLabel);

        if (m_cost > 0.0 && !m_currency.isEmpty()) {
            QLabel *costLabel = new QLabel(QStringLiteral("| Cost: %1 %2").arg(m_cost, 0, 'f', 4).arg(m_currency), usageWidget);
            // Use LinkVisited color (typically purple) for cost display
            QPalette costPal = costLabel->palette();
            costPal.setColor(QPalette::WindowText, costPal.color(QPalette::LinkVisited));
            costLabel->setPalette(costPal);
            costLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            usageLayout->addWidget(costLabel);
        }

        contentLayout->addWidget(usageWidget);
        break;
    }
    case MessageType::PermissionRequest: {
        QWidget *permissionWidget = new QWidget(m_contentWidget);
        QVBoxLayout *permissionLayout = new QVBoxLayout(permissionWidget);
        permissionLayout->setContentsMargins(4, 2, 4, 2);
        permissionLayout->setSpacing(4);

        // Title
        if (!m_permissionTitle.isEmpty()) {
            QLabel *titleLabel = new QLabel(m_permissionTitle, permissionWidget);
            QPalette pal = titleLabel->palette();
            pal.setColor(QPalette::WindowText, pal.color(QPalette::Text));
            titleLabel->setPalette(pal);
            QFont boldFont = titleLabel->font();
            boldFont.setBold(true);
            titleLabel->setFont(boldFont);
            titleLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            titleLabel->setWordWrap(true);
            permissionLayout->addWidget(titleLabel);
        }

        // Command to execute (the full command)
        QLabel *commandLabel = new QLabel(m_permissionCommand, permissionWidget);
        // Use a monospace font
        QFont monoFont(QStringLiteral("monospace"));
        commandLabel->setFont(monoFont);
        // Use alternate background for command display with proper text color
        QPalette cmdPal = commandLabel->palette();
        cmdPal.setColor(QPalette::Base, cmdPal.color(QPalette::AlternateBase));
        // Ensure text is readable on the alternate background
        cmdPal.setColor(QPalette::WindowText, cmdPal.color(QPalette::Text));
        commandLabel->setPalette(cmdPal);
        commandLabel->setAutoFillBackground(true);
        commandLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        commandLabel->setWordWrap(true);
        // Add margins via the label's contents margins
        commandLabel->setContentsMargins(4, 4, 4, 4);
        permissionLayout->addWidget(commandLabel);

        // Buttons
        QWidget *buttonWidget = new QWidget(permissionWidget);
        QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
        buttonLayout->setContentsMargins(0, 4, 0, 0);
        buttonLayout->setSpacing(8);

        QPushButton *allowButton = new QPushButton(i18n("Allow"), buttonWidget);
        allowButton->setProperty("requestId", QVariant::fromValue(m_requestId));
        allowButton->setProperty("optionId", QVariant::fromValue(m_permissionAllowOptionId));
        connect(allowButton, &QPushButton::clicked, this, [this, allowButton]() {
            Q_EMIT permissionResponse(allowButton->property("requestId").toLongLong(), allowButton->property("optionId").toString());
        });
        buttonLayout->addWidget(allowButton);

        QPushButton *rejectButton = new QPushButton(i18n("Reject"), buttonWidget);
        rejectButton->setProperty("requestId", QVariant::fromValue(m_requestId));
        rejectButton->setProperty("optionId", QVariant::fromValue(m_permissionRejectOptionId));
        connect(rejectButton, &QPushButton::clicked, this, [this, rejectButton]() {
            Q_EMIT permissionResponse(rejectButton->property("requestId").toLongLong(), rejectButton->property("optionId").toString());
        });
        buttonLayout->addWidget(rejectButton);

        permissionLayout->addWidget(buttonWidget);
        contentLayout->addWidget(permissionWidget);
        break;
    }
    }

    m_typeLabel->setText(getTypeLabel());
    m_timestampLabel->setText(formatTimestamp(m_timestamp));
}

QString ACPChatMessageWidget::formatTimestamp(const QDateTime &dt) const
{
    return dt.toString(QStringLiteral("hh:mm:ss"));
}

QString ACPChatMessageWidget::getTypeLabel() const
{
    switch (m_type) {
    case MessageType::User:
        return i18n("User");
    case MessageType::Agent:
        return i18n("Agent");
    case MessageType::System:
        return i18n("System");
    case MessageType::Plan:
        return i18n("Plan");
    case MessageType::ToolCall:
        return i18n("Tool Call");
    case MessageType::ToolCallUpdate:
        return i18n("Tool Update");
    case MessageType::Usage:
        return i18n("Usage");
    case MessageType::PermissionRequest:
        return i18n("Permission Request");
    }
    return QString();
}

#include "moc_acpchatmessagewidget.cpp"
