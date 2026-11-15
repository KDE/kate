/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QDateTime>
#include <QJsonValue>
#include <QWidget>

class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class QTextEdit;
class QPushButton;

// Include for MessageStatus enum type
#include "acpclientchatwidget.h"

/**
 * @class ACPChatMessageWidget
 * @brief A widget for displaying a single message in the ACP chat
 *
 * Each message type has distinct visual styling using QPalette colors. Messages display:
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
        Thought, ///< Agent's internal reasoning, collapsible
        Pending, ///< Placeholder shown while the agent works on the prompt
        Queued, ///< User's message waiting for the agent to become free
        System, ///< System messages (gray styling)
        Plan, ///< Plan/step updates (structured display)
        ToolCall, ///< Tool call notification (orange styling)
        ToolCallUpdate, ///< Tool call progress update
        Usage, ///< Token/cost usage information
        PermissionRequest, ///< Inline permission request with Allow/Reject buttons
        Error ///< Error messages (red styling)
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

    /** @brief Set the message status (for prompt turn tracking) */
    void setStatus(ACPClientChatWidget::MessageStatus status);

    /** @brief Get the current message status */
    ACPClientChatWidget::MessageStatus status() const;

    // ========================================================================
    // PERMISSION OPTION STRUCTURE
    // ========================================================================

    /** @brief A permission option from the server */
    struct PermissionOption {
        QString optionId; ///< Unique identifier for this option
        QString name; ///< Human-readable label to display
        QString kind; ///< Kind: allow_once, allow_always, reject_once, reject_always
    };

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
     * @param command Full command line for the tool call
     */
    void setToolCallInfo(const QString &toolCallId, const QString &title, const QString &kind, const QString &status, const QString &command = QString());

    /**
     * @brief Set or update the command for a tool call
     * @param command Full command line for the tool call
     */
    void setToolCallCommand(const QString &command);

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
     * @deprecated Use setPermissionRequestWithOptions() instead
     */
    void setPermissionRequest(qint64 requestId, const QString &title, const QString &command, const QString &allowOptionId, const QString &rejectOptionId);

    /**
     * @brief Set permission request with multiple options (for PermissionRequest message type)
     * @param requestId Permission request identifier
     * @param title Permission request title
     * @param toolName The name/type of the tool being called
     * @param command Full command line to display
     * @param options List of all available permission options from the server
     *
     * Creates inline buttons for each permission option that emit permissionResponse signal.
     * Each option has its own button with the name displayed on it.
     */
    void setPermissionRequestWithOptions(const QJsonValue &requestId,
                                         const QString &title,
                                         const QString &toolName,
                                         const QString &command,
                                         const QList<PermissionOption> &options);

    // ========================================================================
    // ACCESSORS
    // ========================================================================

    /**
     * @brief Show or hide the reasoning of a Thought message
     * @param expanded true to show the whole text
     */
    void setThoughtExpanded(bool expanded);

    /** @brief Get the message type */
    MessageType type() const;

    /** @brief Get the message ID */
    QString messageId() const;

    /** @brief Get the content text */
    QString content() const;

    /** @brief Get the tool call ID (for ToolCall and ToolCallUpdate messages) */
    QString toolCallId() const;

    /** @brief Get the tool command (for ToolCall messages) */
    QString toolCommand() const;

    /** @brief Get the size hint for this widget based on content */
    QSize sizeHint() const override;

Q_SIGNALS:
    /**
     * @brief Emitted when user responds to a permission request
     * @param requestId The permission request ID
     * @param optionId The selected option (allow or reject)
     *
     * Connect to this signal to handle the user's permission decision.
     */
    void permissionResponse(const QJsonValue &requestId, const QString &optionId);

    /**
     * @brief Emitted when the displayed content changed its height
     *
     * The container needs to re-measure the message, for example after the
     * user expanded a collapsed tool call input or output.
     */
    void contentSizeChanged();

protected:
    /** @brief Paint event for custom background drawing */
    void paintEvent(QPaintEvent *event) override;

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /** @brief Set up the widget UI (header, content area, layout) */
    void setupUI();

    /**
     * @brief Apply type-specific styling using QPalette colors
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

    /**
     * @brief Rebuild the content-dependent details of the header
     *
     * The kind, the status and the identifier of a tool call describe the
     * message as a whole, so they are shown next to its type rather than next
     * to its content. They change while a tool call runs, hence the rebuild.
     */
    void updateHeaderDetails();

    /**
     * @brief The glyph and color standing for a tool call status
     * @param status Status reported by the agent
     * @param glyph Output for the glyph to display
     * @param color Output for the color to display it in
     */
    void toolStatusIndicator(const QString &status, QString &glyph, QColor &color) const;

    /**
     * @brief Update the status icon based on the current message status
     *
     * Shows/hides the status icon and sets the appropriate symbol and color.
     */
    void updateStatusIcon();

    /** @brief Get the appropriate background color for this message type */
    QColor getBackgroundColor() const;

    /**
     * @brief The background of code and of tool call input and output
     *
     * Derived from the background of the message, so that it follows the color
     * scheme and stays distinguishable from the message around it.
     */
    QColor codeBackgroundColor() const;

    /** @brief Get the appropriate border color for this message type */
    QColor getBorderColor() const;

    /**
     * @brief Whether this message reports something that went wrong
     *
     * True for error messages and for tool calls that failed. Such messages
     * are accented with the color scheme's negative colors.
     */
    bool isNegative() const;

    // ========================================================================
    // CORE DATA
    // ========================================================================

    MessageType m_type; ///< Message type (set at construction)
    ACPClientChatWidget::MessageStatus m_status = ACPClientChatWidget::MessageStatus::None; ///< Message status (for prompt turn tracking)
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
    QString m_toolCommand; ///< Full command line for tool calls

    /**
     * Expansion state of the collapsible input and output blocks. The content
     * widgets are rebuilt on every tool call update, so the state the user
     * chose has to be remembered here.
     */
    bool m_toolCommandExpanded = false; ///< Whether the tool call input is expanded
    bool m_toolContentExpanded = false; ///< Whether the tool call output is expanded

    // ========================================================================
    // THOUGHT-SPECIFIC DATA
    // ========================================================================

    bool m_thoughtExpanded = false; ///< Whether the reasoning is shown in full

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

    QJsonValue m_requestId; ///< Permission request identifier (can be integer or string)
    QString m_permissionTitle; ///< Permission request title
    QString m_permissionToolName; ///< Name/type of the tool being called
    QString m_permissionCommand; ///< Full command line
    QString m_permissionAllowOptionId; ///< Option ID for allow (deprecated, kept for backward compat)
    QString m_permissionRejectOptionId; ///< Option ID for reject (deprecated, kept for backward compat)
    QList<PermissionOption> m_permissionOptions; ///< All available permission options

    // ========================================================================
    // UI ELEMENTS
    // ========================================================================

    QWidget *m_headerWidget; ///< Container for timestamp, sender, type
    QLabel *m_timestampLabel; ///< Display: formatted timestamp
    QLabel *m_senderLabel; ///< Display: sender name
    QLabel *m_typeLabel; ///< Display: message type label
    QLabel *m_statusIconLabel; ///< Display: status icon (spinner, checkmark, etc.)
    QWidget *m_headerDetailsWidget; ///< Container for content-dependent header details
    QWidget *m_contentWidget; ///< Container for message content
    QVBoxLayout *m_mainLayout; ///< Main widget layout
    QColor m_backgroundColor; ///< Cached background color
    QColor m_borderColor; ///< Cached border color
};
