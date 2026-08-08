/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpchatmessagewidget.h"
#include "acpclient_debug.h"
#include "acpclientprotocol.h"

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
    // Set size policy to prefer expanding in both directions
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 4, 8, 4);
    m_mainLayout->setSpacing(4);

    // Header widget with timestamp, sender, and type
    m_headerWidget = new QWidget(this);
    m_headerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QHBoxLayout *headerLayout = new QHBoxLayout(m_headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    m_timestampLabel = new QLabel(this);
    m_timestampLabel->setObjectName(QStringLiteral("timestampLabel"));
    m_timestampLabel->setTextFormat(Qt::PlainText);
    m_timestampLabel->setWordWrap(true);
    headerLayout->addWidget(m_timestampLabel);

    m_senderLabel = new QLabel(this);
    m_senderLabel->setObjectName(QStringLiteral("senderLabel"));
    m_senderLabel->setTextFormat(Qt::PlainText);
    m_senderLabel->setWordWrap(true);
    QFont boldFont = m_senderLabel->font();
    boldFont.setBold(true);
    m_senderLabel->setFont(boldFont);
    headerLayout->addWidget(m_senderLabel);

    m_typeLabel = new QLabel(this);
    m_typeLabel->setObjectName(QStringLiteral("typeLabel"));
    m_typeLabel->setWordWrap(true);
    headerLayout->addWidget(m_typeLabel);

    // Status icon (for prompt turn tracking)
    m_statusIconLabel = new QLabel(this);
    m_statusIconLabel->setObjectName(QStringLiteral("statusIconLabel"));
    m_statusIconLabel->setVisible(false); // Hidden by default
    headerLayout->addWidget(m_statusIconLabel);

    headerLayout->addStretch();

    m_mainLayout->addWidget(m_headerWidget);

    // Content widget
    m_contentWidget = new QWidget(this);
    m_contentWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
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

void ACPChatMessageWidget::setStatus(ACPClientChatWidget::MessageStatus status)
{
    if (m_status != status) {
        m_status = status;
        updateContentDisplay();
    }
}

ACPClientChatWidget::MessageStatus ACPChatMessageWidget::status() const
{
    return m_status;
}

void ACPChatMessageWidget::updateStatusIcon()
{
    if (!m_statusIconLabel) {
        return;
    }

    // Only show status icon for user messages (prompt turns)
    if (m_type != MessageType::User) {
        m_statusIconLabel->setVisible(false);
        return;
    }

    // Show the icon for user messages based on status
    m_statusIconLabel->setVisible(m_status != ACPClientChatWidget::MessageStatus::None);

    // Set appropriate icon based on status using QPalette for colors
    QPalette pal = m_statusIconLabel->palette();
    switch (m_status) {
    case ACPClientChatWidget::MessageStatus::Running:
        // Show a spinner or loading indicator
        // Using a simple text representation for now: "⏳"
        m_statusIconLabel->setText(QStringLiteral("⏳"));
        break;
    case ACPClientChatWidget::MessageStatus::Completed:
        // Show a checkmark
        m_statusIconLabel->setText(QStringLiteral("✓"));
        pal.setColor(QPalette::WindowText, pal.color(QPalette::LinkVisited));
        m_statusIconLabel->setPalette(pal);
        break;
    case ACPClientChatWidget::MessageStatus::Error:
        // Show an error indicator
        m_statusIconLabel->setText(QStringLiteral("✗"));
        pal.setColor(QPalette::WindowText, pal.color(QPalette::BrightText));
        m_statusIconLabel->setPalette(pal);
        break;
    case ACPClientChatWidget::MessageStatus::Cancelled:
        // Show a cancelled indicator
        m_statusIconLabel->setText(QStringLiteral("○"));
        pal.setColor(QPalette::WindowText, pal.color(QPalette::Mid));
        m_statusIconLabel->setPalette(pal);
        break;
    case ACPClientChatWidget::MessageStatus::None:
    default:
        m_statusIconLabel->setVisible(false);
        break;
    }
}

void ACPChatMessageWidget::addPlanEntry(const QString &content, const QString &priority, const QString &status)
{
    m_planEntries.append({content, priority, status});
    updateContentDisplay();
}

void ACPChatMessageWidget::setToolCallInfo(const QString &toolCallId, const QString &title, const QString &kind, const QString &status, const QString &command)
{
    m_toolCallId = toolCallId;
    m_toolTitle = title;
    m_toolKind = kind;
    m_toolStatus = status;
    m_toolCommand = command;
    updateContentDisplay();
}

void ACPChatMessageWidget::setToolCallCommand(const QString &command)
{
    qCDebug(ACPCLIENT) << "setToolCallCommand called with:" << command << "(was:" << m_toolCommand << ")";
    m_toolCommand = command;
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

    // Convert to new format for backward compatibility
    m_permissionOptions.clear();
    if (!allowOptionId.isEmpty()) {
        m_permissionOptions.append({allowOptionId, i18n("Allow"), ACP::permissionKindAllowOnce()});
    }
    if (!rejectOptionId.isEmpty()) {
        m_permissionOptions.append({rejectOptionId, i18n("Reject"), ACP::permissionKindRejectOnce()});
    }

    updateContentDisplay();
}

void ACPChatMessageWidget::setPermissionRequestWithOptions(qint64 requestId,
                                                           const QString &title,
                                                           const QString &toolName,
                                                           const QString &command,
                                                           const QList<PermissionOption> &options)
{
    m_requestId = requestId;
    m_permissionTitle = title;
    m_permissionToolName = toolName;
    m_permissionCommand = command;
    m_permissionOptions = options;

    // For backward compatibility, set the first allow/reject options found
    m_permissionAllowOptionId.clear();
    m_permissionRejectOptionId.clear();
    for (const PermissionOption &opt : options) {
        if (opt.kind == ACP::permissionKindAllowOnce() || opt.kind == ACP::permissionKindAllowAlways()) {
            if (m_permissionAllowOptionId.isEmpty()) {
                m_permissionAllowOptionId = opt.optionId;
            }
        } else if (opt.kind == ACP::permissionKindRejectOnce() || opt.kind == ACP::permissionKindRejectAlways()) {
            if (m_permissionRejectOptionId.isEmpty()) {
                m_permissionRejectOptionId = opt.optionId;
            }
        }
    }

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

QString ACPChatMessageWidget::toolCallId() const
{
    return m_toolCallId;
}

QString ACPChatMessageWidget::toolCommand() const
{
    return m_toolCommand;
}

void ACPChatMessageWidget::applyTypeSpecificStyling()
{
    // Avoid stylesheets - use only QPalette and QFont
    setAutoFillBackground(true);
}

void ACPChatMessageWidget::updateContentDisplay()
{
    // Update status icon visibility and pixmap
    updateStatusIcon();

    // Clear existing content
    QLayoutItem *child;
    while ((child = m_contentWidget->layout()->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }

    QLayout *contentLayout = m_contentWidget->layout();

    switch (m_type) {
    case MessageType::User:
    case MessageType::Agent:
    case MessageType::System: {
        // Use QLabel with MarkdownText format for native Markdown rendering
        QLabel *contentLabel = new QLabel(m_content, m_contentWidget);
        contentLabel->setWordWrap(true);
        contentLabel->setTextFormat(Qt::MarkdownText);
        contentLabel->setOpenExternalLinks(true);
        contentLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        contentLayout->addWidget(contentLabel);
        break;
    }
    case MessageType::Plan: {
        for (const PlanEntry &entry : m_planEntries) {
            QWidget *entryWidget = new QWidget(m_contentWidget);
            entryWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
            QHBoxLayout *entryLayout = new QHBoxLayout(entryWidget);
            entryLayout->setContentsMargins(4, 2, 4, 2);
            entryLayout->setSpacing(4);

            QLabel *priorityLabel = new QLabel(entry.priority, entryWidget);
            priorityLabel->setWordWrap(true);
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
                statusLabel->setWordWrap(true);
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
        toolWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        QHBoxLayout *toolLayout = new QHBoxLayout(toolWidget);
        toolLayout->setContentsMargins(4, 2, 4, 2);
        toolLayout->setSpacing(4);

        if (!m_toolTitle.isEmpty()) {
            QLabel *titleLabel = new QLabel(m_toolTitle, toolWidget);
            titleLabel->setWordWrap(true);
            QFont boldFont = titleLabel->font();
            boldFont.setBold(true);
            titleLabel->setFont(boldFont);
            titleLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            toolLayout->addWidget(titleLabel);
        }

        if (!m_toolKind.isEmpty()) {
            QLabel *kindLabel = new QLabel(QStringLiteral("(%1)").arg(m_toolKind), toolWidget);
            kindLabel->setWordWrap(true);
            QFont smallFont = kindLabel->font();
            smallFont.setPointSize(smallFont.pointSize() - 2);
            kindLabel->setFont(smallFont);
            kindLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            toolLayout->addWidget(kindLabel);
        }

        // Map status text to appropriate icons
        QString statusText = m_toolStatus;
        if (m_toolStatus == QStringLiteral("in_progress")) {
            statusText = QStringLiteral("⏳"); // Spinner for in progress
        } else if (m_toolStatus == QStringLiteral("pending")) {
            statusText = QStringLiteral("⏳"); // Spinner for pending as well
        } else if (m_toolStatus == QStringLiteral("completed")) {
            statusText = QStringLiteral("✓"); // Checkmark for completed
        } else if (m_toolStatus == QStringLiteral("error") || m_toolStatus == QStringLiteral("failed")) {
            statusText = QStringLiteral("✗"); // Error indicator
        }
        QLabel *statusLabel = new QLabel(statusText, toolWidget);
        statusLabel->setWordWrap(true);
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
            idLabel->setWordWrap(true);
            QFont smallFont = idLabel->font();
            smallFont.setPointSize(smallFont.pointSize() - 2);
            idLabel->setFont(smallFont);
            idLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            toolLayout->addWidget(idLabel);
        }

        // Display command line if available
        if (!m_toolCommand.isEmpty()) {
            QLabel *commandLabel = new QLabel(m_toolCommand, toolWidget);
            // Use a monospace font for command display
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
            toolLayout->addWidget(commandLabel, 1); // Take remaining space
        }

        // Display content if available (from tool call updates)
        if (!m_toolContent.isEmpty()) {
            QLabel *contentLabel = new QLabel(m_toolContent, toolWidget);
            contentLabel->setWordWrap(true);
            contentLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            toolLayout->addWidget(contentLabel, 1); // Take remaining space
        }

        contentLayout->addWidget(toolWidget);
        break;
    }
    case MessageType::ToolCallUpdate: {
        QWidget *updateWidget = new QWidget(m_contentWidget);
        updateWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
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
        statusLabel->setWordWrap(true);
        QFont boldFont = statusLabel->font();
        boldFont.setBold(true);
        statusLabel->setFont(boldFont);
        statusLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        updateLayout->addWidget(statusLabel);

        // Map status text to appropriate icons
        QString statusValueText = m_toolStatus;
        if (m_toolStatus == QStringLiteral("in_progress")) {
            statusValueText = QStringLiteral("⏳"); // Spinner for in progress
        } else if (m_toolStatus == QStringLiteral("pending")) {
            statusValueText = QStringLiteral("⏳"); // Spinner for pending as well
        } else if (m_toolStatus == QStringLiteral("completed")) {
            statusValueText = QStringLiteral("✓"); // Checkmark for completed
        } else if (m_toolStatus == QStringLiteral("error") || m_toolStatus == QStringLiteral("failed")) {
            statusValueText = QStringLiteral("✗"); // Error indicator
        }
        QLabel *statusValueLabel = new QLabel(statusValueText, updateWidget);
        statusValueLabel->setWordWrap(true);
        QFont boldFont2 = statusValueLabel->font();
        boldFont2.setBold(true);
        statusValueLabel->setFont(boldFont2);
        statusValueLabel->setPalette(svPal);
        statusValueLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        updateLayout->addWidget(statusValueLabel);

        if (!m_toolCallId.isEmpty()) {
            QLabel *idLabel = new QLabel(QStringLiteral("[ID: %1]").arg(m_toolCallId), updateWidget);
            idLabel->setWordWrap(true);
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
        usageWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        QHBoxLayout *usageLayout = new QHBoxLayout(usageWidget);
        usageLayout->setContentsMargins(4, 2, 4, 2);
        usageLayout->setSpacing(8);

        QLabel *usageLabel = new QLabel(i18n("Usage:"), usageWidget);
        usageLabel->setWordWrap(true);
        QFont boldFont = usageLabel->font();
        boldFont.setBold(true);
        usageLabel->setFont(boldFont);
        usageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        usageLayout->addWidget(usageLabel);

        QLabel *tokensLabel = new QLabel(QStringLiteral("%1 / %2 tokens").arg(m_usedTokens).arg(m_sizeTokens), usageWidget);
        tokensLabel->setWordWrap(true);
        tokensLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        usageLayout->addWidget(tokensLabel);

        if (m_cost > 0.0 && !m_currency.isEmpty()) {
            QLabel *costLabel = new QLabel(QStringLiteral("| Cost: %1 %2").arg(m_cost, 0, 'f', 4).arg(m_currency), usageWidget);
            costLabel->setWordWrap(true);
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
        permissionWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
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

        // Tool name and command
        QString toolInfo;
        if (!m_permissionToolName.isEmpty()) {
            toolInfo = m_permissionToolName;
            if (!m_permissionCommand.isEmpty()) {
                toolInfo += QStringLiteral(": ") + m_permissionCommand;
            }
        } else if (!m_permissionCommand.isEmpty()) {
            toolInfo = m_permissionCommand;
        }

        if (!toolInfo.isEmpty()) {
            QLabel *commandLabel = new QLabel(toolInfo, permissionWidget);
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
        }

        // Buttons - create a button for each permission option
        QWidget *buttonWidget = new QWidget(permissionWidget);
        buttonWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
        buttonLayout->setContentsMargins(0, 4, 0, 0);
        buttonLayout->setSpacing(8);

        // If we have explicit options, use them
        if (!m_permissionOptions.isEmpty()) {
            for (const PermissionOption &option : m_permissionOptions) {
                QPushButton *button = new QPushButton(option.name, buttonWidget);
                button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                button->setProperty("requestId", QVariant::fromValue(m_requestId));
                button->setProperty("optionId", QVariant::fromValue(option.optionId));

                // Apply styling based on the option kind
                QPalette btnPal = button->palette();
                if (option.kind == ACP::permissionKindAllowAlways()) {
                    // Green-ish for "always allow"
                    btnPal.setColor(QPalette::ButtonText, btnPal.color(QPalette::LinkVisited));
                } else if (option.kind == ACP::permissionKindRejectAlways()) {
                    // Red-ish for "always reject"
                    btnPal.setColor(QPalette::ButtonText, btnPal.color(QPalette::BrightText));
                } else if (option.kind == ACP::permissionKindAllowOnce()) {
                    // Blue-ish for "allow once"
                    btnPal.setColor(QPalette::ButtonText, btnPal.color(QPalette::Link));
                } else if (option.kind == ACP::permissionKindRejectOnce()) {
                    // Orange-ish for "reject once"
                    btnPal.setColor(QPalette::ButtonText, btnPal.color(QPalette::Mid));
                }
                button->setPalette(btnPal);

                connect(button, &QPushButton::clicked, this, [this, button]() {
                    Q_EMIT permissionResponse(button->property("requestId").toLongLong(), button->property("optionId").toString());
                });
                buttonLayout->addWidget(button);
            }
        } else {
            // Fallback to old behavior with just allow/reject buttons
            QPushButton *allowButton = new QPushButton(i18n("Allow"), buttonWidget);
            allowButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            allowButton->setProperty("requestId", QVariant::fromValue(m_requestId));
            allowButton->setProperty("optionId", QVariant::fromValue(m_permissionAllowOptionId));
            connect(allowButton, &QPushButton::clicked, this, [this, allowButton]() {
                Q_EMIT permissionResponse(allowButton->property("requestId").toLongLong(), allowButton->property("optionId").toString());
            });
            buttonLayout->addWidget(allowButton);

            QPushButton *rejectButton = new QPushButton(i18n("Reject"), buttonWidget);
            rejectButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            rejectButton->setProperty("requestId", QVariant::fromValue(m_requestId));
            rejectButton->setProperty("optionId", QVariant::fromValue(m_permissionRejectOptionId));
            connect(rejectButton, &QPushButton::clicked, this, [this, rejectButton]() {
                Q_EMIT permissionResponse(rejectButton->property("requestId").toLongLong(), rejectButton->property("optionId").toString());
            });
            buttonLayout->addWidget(rejectButton);
        }

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
