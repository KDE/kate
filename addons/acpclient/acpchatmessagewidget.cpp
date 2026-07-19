/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#include "acpchatmessagewidget.h"

#include <KLocalizedString>

#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QVBoxLayout>

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

    // Apply type-specific styling
    setStyleSheet(getTypeStyle());
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

            // Priority color
            QString priorityColor;
            if (entry.priority == QStringLiteral("high")) {
                priorityColor = QStringLiteral("#e74c3c");
            } else if (entry.priority == QStringLiteral("medium")) {
                priorityColor = QStringLiteral("#f39c12");
            } else {
                priorityColor = QStringLiteral("#95a5a6");
            }

            QLabel *priorityLabel = new QLabel(entry.priority, entryWidget);
            priorityLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: bold;").arg(priorityColor));
            entryLayout->addWidget(priorityLabel);

            // Status
            if (!entry.status.isEmpty()) {
                QLabel *statusLabel = new QLabel(QStringLiteral("[%1]").arg(entry.status), entryWidget);
                statusLabel->setStyleSheet(QStringLiteral("color: #7f8c8d; font-size: small;"));
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

        // Status color
        QString statusColor;
        if (m_toolStatus == QStringLiteral("pending")) {
            statusColor = QStringLiteral("#f39c12");
        } else if (m_toolStatus == QStringLiteral("in_progress")) {
            statusColor = QStringLiteral("#3498db");
        } else if (m_toolStatus == QStringLiteral("completed")) {
            statusColor = QStringLiteral("#27ae60");
        } else if (m_toolStatus == QStringLiteral("error") || m_toolStatus == QStringLiteral("failed")) {
            statusColor = QStringLiteral("#e74c3c");
        } else {
            statusColor = QStringLiteral("#7f8c8d");
        }

        if (!m_toolTitle.isEmpty()) {
            QLabel *titleLabel = new QLabel(m_toolTitle, toolWidget);
            titleLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #2c3e50;"));
            titleLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            toolLayout->addWidget(titleLabel);
        }

        if (!m_toolKind.isEmpty()) {
            QLabel *kindLabel = new QLabel(QStringLiteral("(%1)").arg(m_toolKind), toolWidget);
            kindLabel->setStyleSheet(QStringLiteral("color: #7f8c8d; font-size: small;"));
            kindLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            toolLayout->addWidget(kindLabel);
        }

        QLabel *statusLabel = new QLabel(m_toolStatus, toolWidget);
        statusLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: bold;").arg(statusColor));
        statusLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        toolLayout->addWidget(statusLabel);

        if (!m_toolCallId.isEmpty()) {
            QLabel *idLabel = new QLabel(QStringLiteral("[ID: %1]").arg(m_toolCallId), toolWidget);
            idLabel->setStyleSheet(QStringLiteral("color: #95a5a6; font-size: small;"));
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

        // Status color
        QString statusColor;
        if (m_toolStatus == QStringLiteral("pending")) {
            statusColor = QStringLiteral("#f39c12");
        } else if (m_toolStatus == QStringLiteral("in_progress")) {
            statusColor = QStringLiteral("#3498db");
        } else if (m_toolStatus == QStringLiteral("completed")) {
            statusColor = QStringLiteral("#27ae60");
        } else {
            statusColor = QStringLiteral("#7f8c8d");
        }

        QLabel *statusLabel = new QLabel(i18n("Tool Update:"), updateWidget);
        statusLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
        statusLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        updateLayout->addWidget(statusLabel);

        QLabel *statusValueLabel = new QLabel(m_toolStatus, updateWidget);
        statusValueLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: bold;").arg(statusColor));
        statusValueLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        updateLayout->addWidget(statusValueLabel);

        if (!m_toolCallId.isEmpty()) {
            QLabel *idLabel = new QLabel(QStringLiteral("[ID: %1]").arg(m_toolCallId), updateWidget);
            idLabel->setStyleSheet(QStringLiteral("color: #95a5a6; font-size: small;"));
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
        usageLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
        usageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        usageLayout->addWidget(usageLabel);

        QLabel *tokensLabel = new QLabel(QStringLiteral("%1 / %2 tokens").arg(m_usedTokens).arg(m_sizeTokens), usageWidget);
        tokensLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        usageLayout->addWidget(tokensLabel);

        if (m_cost > 0.0 && !m_currency.isEmpty()) {
            QLabel *costLabel = new QLabel(QStringLiteral("| Cost: %1 %2").arg(m_cost, 0, 'f', 4).arg(m_currency), usageWidget);
            costLabel->setStyleSheet(QStringLiteral("color: #27ae60;"));
            costLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            usageLayout->addWidget(costLabel);
        }

        contentLayout->addWidget(usageWidget);
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

QString ACPChatMessageWidget::getTypeStyle() const
{
    QString baseStyle = QStringLiteral(
        "ACPChatMessageWidget { "
        "   background-color: #f8f9fa; "
        "   border-radius: 6px; "
        "   padding: 6px; "
        "   margin: 2px 0; "
        "}");

    switch (m_type) {
    case MessageType::User:
        return baseStyle
            + QStringLiteral(
                   "#timestampLabel { color: #7f8c8d; font-size: small; }"
                   "#senderLabel { color: #2c3e50; }"
                   "#typeLabel { color: #2c3e50; }");
    case MessageType::Agent:
        return baseStyle
            + QStringLiteral(
                   "#timestampLabel { color: #7f8c8d; font-size: small; }"
                   "#senderLabel { color: #27ae60; }"
                   "#typeLabel { color: #27ae60; }"
                   "ACPChatMessageWidget { border-left: 3px solid #27ae60; }");
    case MessageType::System:
        return baseStyle
            + QStringLiteral(
                   "#timestampLabel { color: #7f8c8d; font-size: small; }"
                   "#senderLabel { color: #7f8c8d; }"
                   "#typeLabel { color: #7f8c8d; }"
                   "ACPChatMessageWidget { border-left: 3px solid #95a5a6; }");
    case MessageType::Plan:
        return baseStyle
            + QStringLiteral(
                   "#timestampLabel { color: #7f8c8d; font-size: small; }"
                   "#senderLabel { color: #3498db; }"
                   "#typeLabel { color: #3498db; }"
                   "ACPChatMessageWidget { border-left: 3px solid #3498db; background-color: #fff; }");
    case MessageType::ToolCall:
        return baseStyle
            + QStringLiteral(
                   "#timestampLabel { color: #7f8c8d; font-size: small; }"
                   "#senderLabel { color: #f39c12; }"
                   "#typeLabel { color: #f39c12; }"
                   "ACPChatMessageWidget { border-left: 3px solid #f39c12; }");
    case MessageType::ToolCallUpdate:
        return baseStyle
            + QStringLiteral(
                   "#timestampLabel { color: #7f8c8d; font-size: small; }"
                   "#senderLabel { color: #3498db; }"
                   "#typeLabel { color: #3498db; }"
                   "ACPChatMessageWidget { border-left: 3px solid #3498db; background-color: #f0f8ff; }");
    case MessageType::Usage:
        return baseStyle
            + QStringLiteral(
                   "#timestampLabel { color: #7f8c8d; font-size: small; }"
                   "#senderLabel { color: #9b59b6; }"
                   "#typeLabel { color: #9b59b6; }"
                   "ACPChatMessageWidget { border-left: 3px solid #9b59b6; }");
    }

    return baseStyle;
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
    }
    return QString();
}

#include "moc_acpchatmessagewidget.cpp"
