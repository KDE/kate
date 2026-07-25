/*
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QDateTime>
#include <QWidget>

class QLabel;
class QVBoxLayout;
class QHBoxLayout;

/**
 * @class ACPChatMessageWidget
 * @brief A widget for displaying a single message in the ACP chat
 *
 * Each message type has distinct visual styling using CSS. Messages display:
 * - Timestamp
 * - Sender name
 * - Type label
 * - Content (formatted based on type)
 *
 * This widget supports rich content including text, plans, tool calls,
 * usage information, and inline permission requests.
 *
 * @see ACPClientChatWidget for the container widget
 */
class ACPChatMessageWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Message type enumeration
     *
     * Each type has different visual styling and content display.
     */
    enum class MessageType {
        User, ///< User's message (blue styling)
        Agent, ///< Agent's text message (green styling)
        System, ///< System messages (gray styling)
        Plan, ///< Plan/step updates (structured display)
        ToolCall, ///< Tool call notification (orange styling)
        ToolCallUpdate, ///< Tool call progress update
        Usage, ///< Token/cost usage information
        PermissionRequest ///< Inline permission request with Allow/Reject buttons
    };

    /**
     * @brief Construct a message widget
     * @param type Message type (determines styling and content display)
     * @param parent Qt parent widget
     */
    explicit ACPChatMessageWidget(MessageType type, QWidget *parent = nullptr);

    /** @brief Destructor */
    ~ACPChatMessageWidget() override;

    // ========================================================================
    // CONTENT SETTERS
    // ========================================================================

    /**
     * @brief Set the main content text
     * @param content Text to display
     *
     * For most message types, this is the primary display text.
     * The content is formatted based on the message type.
     */
    void setContent(const QString &content);

    /** @brief Set the sender name */
    void setSender(const QString &sender);

    /** @brief Set the timestamp */
    void setTimestamp(const QDateTime &timestamp);

    /** @brief Set the message ID (for tracking streaming chunks) */
    void setMessageId(const QString &messageId);

    // ========================================================================
    // TYPE-SPECIFIC SETTERS
    // ========================================================================

    /**
     * @brief Add a plan entry (for Plan message type)
     * @param content Plan entry text
     * @param priority Priority level ("high", "medium", "low")
     * @param status Entry status
     */
    void addPlanEntry(const QString &content, const QString &priority, const QString &status);

    /**
     * @brief Set tool call information (for ToolCall message type)
     * @param toolCallId Unique tool call identifier
     * @param title Tool name/title
     * @param kind Tool kind/type
     * @param status Current status ("pending", "in_progress", "completed", "error")
     */
    void setToolCallInfo(const QString &toolCallId, const QString &title, const QString &kind, const QString &status);

    /**
     * @brief Set tool call status update (for ToolCallUpdate message type)
     * @param toolCallId Tool call identifier
     * @param status Current status
     * @param content Optional status content text
     */
    void setToolCallStatus(const QString &toolCallId, const QString &status, const QString &content = QString());

    /**
     * @brief Set usage information (for Usage message type)
     * @param used Tokens used so far
     * @param size Total token budget/limit
     * @param cost Cost in currency units
     * @param currency Currency code (e.g., "USD")
     */
    void setUsageInfo(qint64 used, qint64 size, double cost = 0.0, const QString &currency = QString());

    /**
     * @brief Set permission request (for PermissionRequest message type)
     * @param requestId Permission request identifier
     * @param title Permission request title
     * @param command Full command line to display
     * @param allowOptionId Option ID for allow action
     * @param rejectOptionId Option ID for reject action
     *
     * Creates inline Allow/Reject buttons that emit permissionResponse signal.
     */
    void setPermissionRequest(qint64 requestId, const QString &title, const QString &command, const QString &allowOptionId, const QString &rejectOptionId);

    // ========================================================================
    // ACCESSORS
    // ========================================================================

    /** @brief Get the message type */
    MessageType type() const;

    /** @brief Get the message ID */
    QString messageId() const;

    /** @brief Get the content text */
    QString content() const;

Q_SIGNALS:
    /**
     * @brief Emitted when user responds to a permission request
     * @param requestId The permission request ID
     * @param optionId The selected option (allow or reject)
     *
     * Connect to this signal to handle the user's permission decision.
     */
    void permissionResponse(qint64 requestId, const QString &optionId);

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /** @brief Set up the widget UI (header, content area, layout) */
    void setupUI();

    /**
     * @brief Apply basic styling without hardcoded colors
     *
     * Uses QPalette and QFont instead of stylesheets for proper theming.
     */
    void applyTypeSpecificStyling();

    /** @brief Format a timestamp for display */
    QString formatTimestamp(const QDateTime &dt) const;

    /** @brief Get the type label text */
    QString getTypeLabel() const;

    /**
     * @brief Update the content display based on message type and data
     *
     * Called automatically when setters are called. Rebuilds the content area
     * with appropriate widgets for the current message type.
     */
    void updateContentDisplay();

    // ========================================================================
    // CORE DATA
    // ========================================================================

    MessageType m_type; ///< Message type (set at construction)
    QString m_content; ///< Main content text
    QString m_sender; ///< Sender name (e.g., "You", "Agent", "System")
    QDateTime m_timestamp; ///< Timestamp for the message
    QString m_messageId; ///< Unique message ID (for streaming chunk matching)

    // ========================================================================
    // PLAN-SPECIFIC DATA
    // ========================================================================

    /** @brief A single plan entry */
    struct PlanEntry {
        QString content; ///< Entry text
        QString priority; ///< Priority level
        QString status; ///< Entry status
    };
    QList<PlanEntry> m_planEntries; ///< List of plan entries for Plan message type

    // ========================================================================
    // TOOL CALL-SPECIFIC DATA
    // ========================================================================

    QString m_toolCallId; ///< Unique tool call identifier
    QString m_toolTitle; ///< Tool name/title
    QString m_toolKind; ///< Tool kind/type
    QString m_toolStatus; ///< Current status
    QString m_toolContent; ///< Status content text

    // ========================================================================
    // USAGE-SPECIFIC DATA
    // ========================================================================

    qint64 m_usedTokens; ///< Tokens used so far
    qint64 m_sizeTokens; ///< Total token budget
    double m_cost; ///< Cost in currency units
    QString m_currency; ///< Currency code

    // ========================================================================
    // PERMISSION REQUEST DATA
    // ========================================================================

    qint64 m_requestId; ///< Permission request identifier
    QString m_permissionTitle; ///< Permission request title
    QString m_permissionCommand; ///< Full command line
    QString m_permissionAllowOptionId; ///< Option ID for allow
    QString m_permissionRejectOptionId; ///< Option ID for reject

    // ========================================================================
    // UI ELEMENTS
    // ========================================================================

    QWidget *m_headerWidget; ///< Container for timestamp, sender, type
    QLabel *m_timestampLabel; ///< Display: formatted timestamp
    QLabel *m_senderLabel; ///< Display: sender name
    QLabel *m_typeLabel; ///< Display: message type label
    QWidget *m_contentWidget; ///< Container for message content
    QVBoxLayout *m_mainLayout; ///< Main widget layout
};
