/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QDateTime>
#include <QWidget>

class QLabel;
class QVBoxLayout;
class QHBoxLayout;

/**
 * A single message widget for the ACP chat display.
 * Each message type has distinct visual styling.
 */
class ACPChatMessageWidget : public QWidget
{
    Q_OBJECT

public:
    enum class MessageType {
        User, // User's message
        Agent, // Agent's text message
        System, // System messages
        Plan, // Plan updates
        ToolCall, // Tool call notification
        ToolCallUpdate, // Tool call progress update
        Usage, // Usage updates
        PermissionRequest // Permission request with buttons
    };

    explicit ACPChatMessageWidget(MessageType type, QWidget *parent = nullptr);
    ~ACPChatMessageWidget() override;

    // Set message content
    void setContent(const QString &content);
    void setSender(const QString &sender);
    void setTimestamp(const QDateTime &timestamp);
    void setMessageId(const QString &messageId);

    // For plan messages
    void addPlanEntry(const QString &content, const QString &priority, const QString &status);

    // For tool call messages
    void setToolCallInfo(const QString &toolCallId, const QString &title, const QString &kind, const QString &status);

    // For tool call updates
    void setToolCallStatus(const QString &toolCallId, const QString &status, const QString &content = QString());

    // For usage updates
    void setUsageInfo(qint64 used, qint64 size, double cost = 0.0, const QString &currency = QString());

    // For permission requests
    void setPermissionRequest(qint64 requestId, const QString &title, const QString &command, const QString &allowOptionId, const QString &rejectOptionId);

    MessageType type() const;
    QString messageId() const;
    QString content() const;

Q_SIGNALS:
    void permissionResponse(qint64 requestId, const QString &optionId);

private:
    void setupUI();
    QString formatTimestamp(const QDateTime &dt) const;
    QString getTypeStyle() const;
    QString getTypeLabel() const;
    void updateContentDisplay();

    MessageType m_type;
    QString m_content;
    QString m_sender;
    QDateTime m_timestamp;
    QString m_messageId;

    // Plan-specific data
    struct PlanEntry {
        QString content;
        QString priority;
        QString status;
    };
    QList<PlanEntry> m_planEntries;

    // Tool call-specific data
    QString m_toolCallId;
    QString m_toolTitle;
    QString m_toolKind;
    QString m_toolStatus;
    QString m_toolContent;

    // Usage-specific data
    qint64 m_usedTokens;
    qint64 m_sizeTokens;
    double m_cost;
    QString m_currency;

    // Permission request data
    qint64 m_requestId;
    QString m_permissionTitle;
    QString m_permissionCommand;
    QString m_permissionAllowOptionId;
    QString m_permissionRejectOptionId;

    // UI elements
    QWidget *m_headerWidget;
    QLabel *m_timestampLabel;
    QLabel *m_senderLabel;
    QLabel *m_typeLabel;
    QWidget *m_contentWidget;
    QVBoxLayout *m_mainLayout;
};
