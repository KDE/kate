/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpchatmessagewidget.h"
#include "acpclient_debug.h"
#include "acpclientprotocol.h"
#include "acpcollapsibletextwidget.h"

#include <KColorScheme>
#include <KLocalizedString>
#include <KSqueezedTextLabel>

#include <QApplication>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QProgressBar>
#include <QPushButton>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextTable>
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
    applyTypeSpecificStyling();
    updateContentDisplay();
}

ACPChatMessageWidget::~ACPChatMessageWidget()
{
    // Widgets are deleted automatically by Qt's parent-child system
}

void ACPChatMessageWidget::setupUI()
{
    // Set size policy to expand horizontally for proper word wrapping
    // Use Minimum vertically to avoid excessive height
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(12, 8, 12, 8);
    m_mainLayout->setSpacing(6);

    // Header widget with timestamp, sender, and type
    m_headerWidget = new QWidget(this);
    m_headerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QHBoxLayout *headerLayout = new QHBoxLayout(m_headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(12);

    m_timestampLabel = new QLabel(this);
    m_timestampLabel->setObjectName(QStringLiteral("timestampLabel"));
    m_timestampLabel->setTextFormat(Qt::PlainText);
    m_timestampLabel->setWordWrap(false);
    QPalette timePal = m_timestampLabel->palette();
    timePal.setColor(QPalette::WindowText, timePal.color(QPalette::Mid));
    m_timestampLabel->setPalette(timePal);
    QFont timeFont = m_timestampLabel->font();
    timeFont.setPointSize(timeFont.pointSize() - 1);
    m_timestampLabel->setFont(timeFont);
    headerLayout->addWidget(m_timestampLabel);

    m_senderLabel = new QLabel(this);
    m_senderLabel->setObjectName(QStringLiteral("senderLabel"));
    m_senderLabel->setTextFormat(Qt::PlainText);
    m_senderLabel->setWordWrap(false);
    QFont boldFont = m_senderLabel->font();
    boldFont.setBold(true);
    m_senderLabel->setFont(boldFont);
    headerLayout->addWidget(m_senderLabel);

    m_typeLabel = new QLabel(this);
    m_typeLabel->setObjectName(QStringLiteral("typeLabel"));
    m_typeLabel->setWordWrap(false);
    QFont typeFont = m_typeLabel->font();
    typeFont.setPointSize(typeFont.pointSize() - 1);
    m_typeLabel->setFont(typeFont);
    headerLayout->addWidget(m_typeLabel);

    // Status icon (for prompt turn tracking)
    m_statusIconLabel = new QLabel(this);
    m_statusIconLabel->setObjectName(QStringLiteral("statusIconLabel"));
    m_statusIconLabel->setVisible(false); // Hidden by default
    m_statusIconLabel->setWordWrap(false);
    headerLayout->addWidget(m_statusIconLabel);

    // Details that depend on the content of the message, such as the kind, the
    // status and the identifier of a tool call. They belong next to the type of
    // the message rather than next to its content, and are filled in by
    // updateHeaderDetails() because the content can change.
    m_headerDetailsWidget = new QWidget(m_headerWidget);
    QHBoxLayout *headerDetailsLayout = new QHBoxLayout(m_headerDetailsWidget);
    headerDetailsLayout->setContentsMargins(0, 0, 0, 0);
    headerDetailsLayout->setSpacing(8);
    m_headerDetailsWidget->setVisible(false);
    headerLayout->addWidget(m_headerDetailsWidget);

    headerLayout->addStretch();

    m_mainLayout->addWidget(m_headerWidget);

    // Content widget
    m_contentWidget = new QWidget(this);
    m_contentWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(4);
    m_mainLayout->addWidget(m_contentWidget);

    // A message is given the height it was measured with, but a measurement
    // taken before the layout settled can come out too large. Collect that
    // slack here rather than letting it stretch the header, which would space
    // the timestamp, the sender and the type of a message apart for no reason.
    m_mainLayout->addStretch();

    // Set initial values
    m_timestampLabel->setText(formatTimestamp(m_timestamp));
    m_typeLabel->setText(getTypeLabel());
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
        updateStatusIcon();
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
        m_statusIconLabel->setText(QStringLiteral("⏳"));
        pal.setColor(QPalette::WindowText, pal.color(QPalette::Link));
        m_statusIconLabel->setPalette(pal);
        break;
    case ACPClientChatWidget::MessageStatus::Completed:
        m_statusIconLabel->setText(QStringLiteral("✓"));
        pal.setColor(QPalette::WindowText, pal.color(QPalette::LinkVisited));
        m_statusIconLabel->setPalette(pal);
        break;
    case ACPClientChatWidget::MessageStatus::Error:
        m_statusIconLabel->setText(QStringLiteral("✗"));
        pal.setColor(QPalette::WindowText, pal.color(QPalette::BrightText));
        m_statusIconLabel->setPalette(pal);
        break;
    case ACPClientChatWidget::MessageStatus::Cancelled:
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

void ACPChatMessageWidget::setThoughtExpanded(bool expanded)
{
    if (m_type != MessageType::Thought || m_thoughtExpanded == expanded) {
        return;
    }

    m_thoughtExpanded = expanded;
    updateContentDisplay();
    Q_EMIT contentSizeChanged();
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

void ACPChatMessageWidget::setPermissionRequestWithOptions(const QJsonValue &requestId,
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

QSize ACPChatMessageWidget::sizeHint() const
{
    // Calculate size hint based on content
    // This helps with proper layout in the list widget
    QSize baseSize = QWidget::sizeHint();

    // For messages with content, ensure minimum width for readability
    if (!m_content.isEmpty() || m_type == MessageType::PermissionRequest) {
        // Get the maximum width we might need
        int maxWidth = 600; // Reasonable maximum for chat messages

        // If we have a parent, try to use its width
        if (parentWidget()) {
            maxWidth = qMin(maxWidth, parentWidget()->width() - 24);
        }

        // Ensure we have at least some width
        baseSize.setWidth(qMax(baseSize.width(), maxWidth));
    }

    return baseSize;
}

void ACPChatMessageWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Get colors based on message type
    const QColor bgColor = getBackgroundColor();
    const QColor borderColor = getBorderColor();

    // Failures get a heavier border so they stand out while scrolling
    const qreal borderWidth = isNegative() ? 2.0 : 1.0;

    // Inset by half the pen width, otherwise the outer half of the border is
    // drawn outside the widget and clipped away
    const QRectF rect = QRectF(this->rect()).adjusted(borderWidth / 2.0, borderWidth / 2.0, -borderWidth / 2.0, -borderWidth / 2.0);

    // Draw background with rounded corners
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(rect, 8, 8);

    // Draw border
    painter.setPen(QPen(borderColor, borderWidth));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect, 8, 8);
}

/**
 * @brief Mix two colors
 * @param base Color to start from
 * @param accent Color to mix in
 * @param weight Share of @p accent, between 0 and 1
 */
static QColor mixColors(const QColor &base, const QColor &accent, qreal weight)
{
    const qreal rest = 1.0 - weight;
    return QColor::fromRgbF(base.redF() * rest + accent.redF() * weight,
                            base.greenF() * rest + accent.greenF() * weight,
                            base.blueF() * rest + accent.blueF() * weight);
}

/** @brief Whether a piece of text was rendered as code by the Markdown parser */
static bool isCodeFormat(const QTextCharFormat &format)
{
    const QFont font = format.font();
    if (font.fixedPitch()) {
        return true;
    }

    const QStringList families = font.families();
    for (const QString &family : families) {
        if (family.compare(QStringLiteral("monospace"), Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Render Markdown with the code given a background of its own
 * @param markdown Message text, may be plain text
 * @param codeBackground Background to paint behind code
 * @return Rich text to display, empty if the text contains no code
 *
 * QLabel cannot style parts of the Markdown it renders itself, so the text is
 * parsed into a document here, which lets the code be painted with a
 * background before the result is handed to the label as rich text.
 *
 * Fenced blocks are moved into a one cell table, which is the only container
 * Qt lays out with padding inside its background and whose padding survives
 * being written out as rich text. A background set on the blocks themselves
 * starts right at the first letter, and neither block margins, block indent
 * nor the padding of a text frame change that.
 *
 * Only fenced blocks can be found this way. Qt's Markdown parser does not mark
 * code that is indented by four spaces, and that form is indistinguishable
 * from prose afterwards.
 */
static QString markdownWithHighlightedCode(const QString &markdown, const QColor &codeBackground)
{
    QTextDocument document;
    document.setMarkdown(markdown);

    bool foundCode = false;

    // Collect the code spans of the running text before anything is changed,
    // because editing the document invalidates the iterators walking it
    QList<QPair<int, int>> inlineRanges;
    QList<QPair<int, int>> fencedRuns;
    int runStart = -1;
    int runEnd = -1;

    for (QTextBlock block = document.begin(); block.isValid(); block = block.next()) {
        if (block.blockFormat().hasProperty(QTextFormat::BlockCodeFence)) {
            // Consecutive fenced blocks are the lines of one code block
            if (runStart < 0) {
                runStart = block.blockNumber();
            }
            runEnd = block.blockNumber();
            continue;
        }

        if (runStart >= 0) {
            fencedRuns.append({runStart, runEnd});
            runStart = -1;
        }

        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (fragment.isValid() && isCodeFormat(fragment.charFormat())) {
                inlineRanges.append({fragment.position(), fragment.length()});
            }
        }
    }

    if (runStart >= 0) {
        fencedRuns.append({runStart, runEnd});
    }

    // Inline code only needs the background, it is part of a line of prose
    for (const QPair<int, int> &range : std::as_const(inlineRanges)) {
        QTextCursor cursor(&document);
        cursor.setPosition(range.first);
        cursor.setPosition(range.first + range.second, QTextCursor::KeepAnchor);

        QTextCharFormat format;
        format.setBackground(codeBackground);
        cursor.mergeCharFormat(format);
        foundCode = true;
    }

    // Replace the runs from the back, so that the positions of the ones still
    // to be replaced stay valid
    for (int i = fencedRuns.size() - 1; i >= 0; --i) {
        const QTextBlock first = document.findBlockByNumber(fencedRuns.at(i).first);
        const QTextBlock last = document.findBlockByNumber(fencedRuns.at(i).second);
        if (!first.isValid() || !last.isValid()) {
            continue;
        }

        QStringList code;
        for (QTextBlock block = first; block.isValid(); block = block.next()) {
            code.append(block.text());
            if (block.blockNumber() == last.blockNumber()) {
                break;
            }
        }

        QTextCursor cursor(&document);
        cursor.setPosition(first.position());
        cursor.setPosition(last.position() + last.length() - 1, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();

        QTextTableFormat tableFormat;
        tableFormat.setBorder(0);
        tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_None);
        tableFormat.setCellSpacing(0);
        tableFormat.setCellPadding(6);
        tableFormat.setWidth(QTextLength(QTextLength::PercentageLength, 100));
        tableFormat.setTopMargin(4);
        tableFormat.setBottomMargin(4);

        QTextTable *table = cursor.insertTable(1, 1, tableFormat);

        // The background belongs on the cell. A table paints it behind the
        // cells only, so the padding would stay in the color of the message.
        QTextTableCell cell = table->cellAt(0, 0);
        QTextCharFormat cellFormat = cell.format();
        cellFormat.setBackground(codeBackground);
        cell.setFormat(cellFormat);

        QTextCharFormat codeFormat;
        codeFormat.setFontFamilies({QStringLiteral("monospace")});
        codeFormat.setFontFixedPitch(true);

        QTextCursor cellCursor = cell.firstCursorPosition();
        cellCursor.setCharFormat(codeFormat);
        for (int line = 0; line < code.size(); ++line) {
            if (line > 0) {
                cellCursor.insertBlock(cellCursor.blockFormat(), codeFormat);
            }
            cellCursor.insertText(code.at(line));
        }

        foundCode = true;
    }

    // Leave text without code to the Markdown renderer of the label, which
    // avoids a conversion to rich text for most messages
    if (!foundCode) {
        return QString();
    }

    return document.toHtml();
}

/**
 * @brief Keep a short label of a header row on one line
 *
 * The kind, the status and the identifier of a tool call belong next to its
 * title. They are short, but they were word wrapped, so the layout was free to
 * break them at any space they contain: "[ID: 42]" ended up as "[ID:" with the
 * identifier on a line of its own.
 */
static void keepOnOneLine(QLabel *label)
{
    label->setWordWrap(false);

    // Never give up width that the title can use instead
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
}

/**
 * @brief Create the label showing the identifier of a tool call
 * @param toolCallId Identifier to display
 * @param parent Parent widget, which the label is added to
 *
 * Identifiers are generated, so they are of no interest until one has to be
 * matched against a log. The label is elided in the middle, which keeps the
 * prefix and the tail of an identifier readable, and the whole identifier is
 * available as its tooltip.
 */
static KSqueezedTextLabel *createToolCallIdLabel(const QString &toolCallId, QWidget *parent)
{
    KSqueezedTextLabel *idLabel = new KSqueezedTextLabel(QStringLiteral("[ID: %1]").arg(toolCallId), parent);
    idLabel->setTextElideMode(Qt::ElideMiddle);
    idLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    QFont smallFont = idLabel->font();
    smallFont.setPointSize(qMax(1, smallFont.pointSize() - 2));
    idLabel->setFont(smallFont);

    // Long identifiers must not push the rest of the row out of the message
    static constexpr int maxIdCharsShown = 28;
    idLabel->setMaximumWidth(idLabel->fontMetrics().averageCharWidth() * maxIdCharsShown);

    return idLabel;
}

void ACPChatMessageWidget::toolStatusIndicator(const QString &status, QString &glyph, QColor &color) const
{
    glyph = status;
    color = palette().color(QPalette::Text);

    if (status == QStringLiteral("pending") || status == QStringLiteral("in_progress")) {
        glyph = QStringLiteral("⏳");
        color = palette().color(QPalette::Link);
    } else if (status == QStringLiteral("completed")) {
        glyph = QStringLiteral("✓");
        color = palette().color(QPalette::LinkVisited);
    } else if (status == QStringLiteral("error") || status == QStringLiteral("failed")) {
        glyph = QStringLiteral("✗");
        color = KColorScheme().foreground(KColorScheme::NegativeText).color();
    }
}

void ACPChatMessageWidget::updateHeaderDetails()
{
    QLayout *detailsLayout = m_headerDetailsWidget->layout();

    QLayoutItem *child;
    while ((child = detailsLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    if (m_type != MessageType::ToolCall && m_type != MessageType::ToolCallUpdate) {
        m_headerDetailsWidget->setVisible(false);
        return;
    }

    if (!m_toolKind.isEmpty()) {
        QLabel *kindLabel = new QLabel(QStringLiteral("(%1)").arg(m_toolKind), m_headerDetailsWidget);
        keepOnOneLine(kindLabel);
        QFont smallFont = kindLabel->font();
        smallFont.setPointSize(qMax(1, smallFont.pointSize() - 1));
        kindLabel->setFont(smallFont);
        detailsLayout->addWidget(kindLabel);
    }

    if (!m_toolStatus.isEmpty()) {
        QString glyph;
        QColor color;
        toolStatusIndicator(m_toolStatus, glyph, color);

        QLabel *statusLabel = new QLabel(glyph, m_headerDetailsWidget);
        keepOnOneLine(statusLabel);
        QFont boldFont = statusLabel->font();
        boldFont.setBold(true);
        statusLabel->setFont(boldFont);
        QPalette statusPal = statusLabel->palette();
        statusPal.setColor(QPalette::WindowText, color);
        statusLabel->setPalette(statusPal);
        statusLabel->setToolTip(m_toolStatus);
        detailsLayout->addWidget(statusLabel);
    }

    if (!m_toolCallId.isEmpty()) {
        detailsLayout->addWidget(createToolCallIdLabel(m_toolCallId, m_headerDetailsWidget));
    }

    const bool hasDetails = detailsLayout->count() > 0;
    m_headerDetailsWidget->setVisible(hasDetails);

    // Newly created widgets are only shown once the event loop ran, see
    // updateContentDisplay()
    if (hasDetails && !isHidden()) {
        for (int i = 0; i < detailsLayout->count(); ++i) {
            if (QWidget *detail = detailsLayout->itemAt(i)->widget()) {
                detail->show();
            }
        }
    }
}

bool ACPChatMessageWidget::isNegative() const
{
    if (m_type == MessageType::Error) {
        return true;
    }

    if (m_type != MessageType::ToolCall && m_type != MessageType::ToolCallUpdate) {
        return false;
    }

    // "failed" is the status defined by ACP, "error" is accepted as well
    // because agents have been seen to report it
    return m_toolStatus == QStringLiteral("failed") || m_toolStatus == QStringLiteral("error");
}

QColor ACPChatMessageWidget::codeBackgroundColor() const
{
    // Mixed into the background of the message rather than taken from
    // QPalette::AlternateBase, which differs from a message background by a
    // contrast ratio of about 1.03 and cannot be seen
    return mixColors(getBackgroundColor(), palette().color(QPalette::Text), 0.13);
}

QColor ACPChatMessageWidget::getBackgroundColor() const
{
    QPalette pal = palette();

    // Tint failures red. The tint is mixed into the normal background instead
    // of using KColorScheme::NegativeBackground, because that color is meant
    // to be combined with the color scheme's own text color while the message
    // text follows the widget palette. Mixing keeps the contrast of the theme.
    if (isNegative()) {
        return mixColors(pal.color(QPalette::Base), KColorScheme().foreground(KColorScheme::NegativeText).color(), 0.12);
    }

    // Use palette colors that work in both light and dark modes
    // Base colors that adapt to the theme
    QColor baseColor = pal.color(QPalette::Base);
    QColor alternateBase = pal.color(QPalette::AlternateBase);

    // Calculate colors based on palette for theme compatibility
    switch (m_type) {
    case MessageType::User:
        // User messages: slightly tinted with highlight color
        return baseColor.lighter(105);
    case MessageType::Agent:
        // Agent messages: slightly tinted with link color (green/blue depending on theme)
        return baseColor.lighter(103);
    case MessageType::Thought:
        // Reasoning is secondary, so it stays closer to the surroundings
        return alternateBase;
    case MessageType::Pending:
        // Placeholder, kept quiet until it is replaced by real content
        return alternateBase;
    case MessageType::Queued:
        // Not sent yet, so it is toned down compared to a user message
        return alternateBase;
    case MessageType::System:
        // System messages: alternate base color
        return alternateBase;
    case MessageType::Plan:
        // Plan messages: slightly lighter than base
        return baseColor.lighter(102);
    case MessageType::ToolCall:
        // Tool call messages: slightly darker than base
        return baseColor.darker(102);
    case MessageType::ToolCallUpdate:
        // Tool call update messages: same as tool call but slightly different
        return baseColor.darker(101);
    case MessageType::Usage:
        // Usage messages: alternate base
        return alternateBase.lighter(102);
    case MessageType::PermissionRequest:
        // Permission request messages: slightly tinted
        return baseColor.lighter(104);
    case MessageType::Error:
        // Handled by isNegative() above
        return baseColor;
    default:
        return baseColor;
    }
}

QColor ACPChatMessageWidget::getBorderColor() const
{
    if (isNegative()) {
        return KColorScheme().foreground(KColorScheme::NegativeText).color();
    }

    QPalette pal = palette();

    // Use palette colors for borders that work in both light and dark modes
    switch (m_type) {
    case MessageType::User:
        return pal.color(QPalette::Link);
    case MessageType::Agent:
        return pal.color(QPalette::LinkVisited);
    case MessageType::Thought:
        return pal.color(QPalette::Mid);
    case MessageType::Pending:
        return pal.color(QPalette::Mid);
    case MessageType::Queued:
        return pal.color(QPalette::Link);
    case MessageType::System:
        return pal.color(QPalette::Mid);
    case MessageType::Plan:
        return pal.color(QPalette::Text).lighter(150);
    case MessageType::ToolCall:
    case MessageType::ToolCallUpdate:
        return pal.color(QPalette::Text).lighter(130);
    case MessageType::Usage:
        return pal.color(QPalette::Text).lighter(140);
    case MessageType::PermissionRequest:
        return pal.color(QPalette::Text).lighter(120);
    case MessageType::Error:
        // Handled by isNegative() above
        return pal.color(QPalette::Mid);
    default:
        return pal.color(QPalette::Mid);
    }
}

void ACPChatMessageWidget::applyTypeSpecificStyling()
{
    // Set background and border colors
    m_backgroundColor = getBackgroundColor();
    m_borderColor = getBorderColor();

    switch (m_type) {
    case MessageType::User:
    case MessageType::Agent:
    case MessageType::Thought:
    case MessageType::Pending:
    case MessageType::Queued:
    case MessageType::System:
    case MessageType::Plan:
    case MessageType::ToolCall:
    case MessageType::ToolCallUpdate:
    case MessageType::Usage:
    case MessageType::PermissionRequest:
    case MessageType::Error:
        // Use default text color
        break;
    }
}

void ACPChatMessageWidget::updateContentDisplay()
{
    // Update status icon visibility and pixmap
    updateStatusIcon();

    // Parts of the header describe the content, so they follow it
    updateHeaderDetails();

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
    case MessageType::Queued:
    case MessageType::Agent:
    case MessageType::System:
    case MessageType::Error: {
        QLabel *contentLabel = new QLabel(m_contentWidget);
        contentLabel->setWordWrap(true);
        contentLabel->setOpenExternalLinks(true);
        contentLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

        // Give code blocks a background of their own, so that they can be told
        // apart from the prose around them. The tint is derived from the
        // background of this message, which keeps it theme independent.
        const QString highlighted = markdownWithHighlightedCode(m_content, codeBackgroundColor());
        if (highlighted.isEmpty()) {
            // No code blocks, let the label render the Markdown itself
            contentLabel->setTextFormat(Qt::MarkdownText);
            contentLabel->setText(m_content);
        } else {
            contentLabel->setTextFormat(Qt::RichText);
            contentLabel->setText(highlighted);
        }

        // Set appropriate text color for error messages
        if (m_type == MessageType::Error) {
            QPalette errorPal = contentLabel->palette();
            errorPal.setColor(QPalette::WindowText, errorPal.color(QPalette::BrightText));
            contentLabel->setPalette(errorPal);
        }

        contentLayout->addWidget(contentLabel);
        break;
    }
    case MessageType::Pending: {
        QWidget *pendingWidget = new QWidget(m_contentWidget);
        pendingWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        QHBoxLayout *pendingLayout = new QHBoxLayout(pendingWidget);
        pendingLayout->setContentsMargins(0, 0, 0, 0);
        pendingLayout->setSpacing(8);

        // An indeterminate progress bar animates itself and follows the style,
        // which avoids both a timer and a hand drawn spinner
        QProgressBar *busyIndicator = new QProgressBar(pendingWidget);
        busyIndicator->setRange(0, 0);
        busyIndicator->setTextVisible(false);
        busyIndicator->setFixedWidth(120);
        busyIndicator->setFixedHeight(qMax(6, fontMetrics().height() / 2));
        pendingLayout->addWidget(busyIndicator);

        QLabel *pendingLabel = new QLabel(m_content.isEmpty() ? i18n("Waiting for the agent…") : m_content, pendingWidget);
        pendingLabel->setWordWrap(true);
        QPalette pendingPal = pendingLabel->palette();
        pendingPal.setColor(QPalette::WindowText, pendingPal.color(QPalette::Mid));
        pendingLabel->setPalette(pendingPal);
        pendingLayout->addWidget(pendingLabel, 1);

        contentLayout->addWidget(pendingWidget);
        break;
    }
    case MessageType::Thought: {
        // Reasoning is secondary, so it is collapsible no matter how short it
        // is, and the chat decides whether it starts out expanded
        ACPCollapsibleTextWidget *thoughtWidget = new ACPCollapsibleTextWidget(i18n("Reasoning"), m_contentWidget);
        thoughtWidget->setCollapseMode(ACPCollapsibleTextWidget::CollapseMode::HideAll);
        thoughtWidget->setExpanded(m_thoughtExpanded);
        thoughtWidget->setText(m_content);
        connect(thoughtWidget, &ACPCollapsibleTextWidget::expandedChanged, this, [this](bool expanded) {
            m_thoughtExpanded = expanded;
            Q_EMIT contentSizeChanged();
        });

        contentLayout->addWidget(thoughtWidget);
        break;
    }
    case MessageType::Plan: {
        for (const PlanEntry &entry : std::as_const(m_planEntries)) {
            QWidget *entryWidget = new QWidget(m_contentWidget);
            entryWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
            QHBoxLayout *entryLayout = new QHBoxLayout(entryWidget);
            entryLayout->setContentsMargins(4, 2, 4, 2);
            entryLayout->setSpacing(8);

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
        QVBoxLayout *toolLayout = new QVBoxLayout(toolWidget);
        toolLayout->setContentsMargins(4, 2, 4, 2);
        // Keep the input and output blocks visibly apart
        toolLayout->setSpacing(8);

        // First row: the title. The kind, the status and the identifier
        // describe the message as a whole and live in its header.
        if (!m_toolTitle.isEmpty()) {
            QWidget *firstRow = new QWidget(toolWidget);
            QHBoxLayout *firstRowLayout = new QHBoxLayout(firstRow);
            firstRowLayout->setContentsMargins(0, 0, 0, 0);
            firstRowLayout->setSpacing(8);

            // Agents commonly put the whole command into the title, which the
            // input block below already shows in full. Keep the title on one
            // line and elide it, so that it never dominates the message.
            // Guard against pathological titles, eliding those costs a text
            // layout of the whole string on every resize.
            static constexpr int maxTitleLength = 2000;
            QString titleText = m_toolTitle.simplified();
            if (titleText.size() > maxTitleLength) {
                titleText = titleText.left(maxTitleLength) + QStringLiteral("…");
            }

            KSqueezedTextLabel *titleLabel = new KSqueezedTextLabel(titleText, firstRow);
            titleLabel->setTextElideMode(Qt::ElideRight);
            QFont boldFont = titleLabel->font();
            boldFont.setBold(true);
            titleLabel->setFont(boldFont);
            titleLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

            // Show no more than this many characters even in a wide window,
            // the rest of the header row stays readable that way
            static constexpr int maxTitleCharsShown = 60;
            titleLabel->setMaximumWidth(titleLabel->fontMetrics().averageCharWidth() * maxTitleCharsShown);

            firstRowLayout->addWidget(titleLabel);
            firstRowLayout->addStretch();
            toolLayout->addWidget(firstRow);
        }

        // Second row: command line if available
        if (!m_toolCommand.isEmpty()) {
            ACPCollapsibleTextWidget *commandWidget = new ACPCollapsibleTextWidget(i18n("Input"), toolWidget);
            commandWidget->setMonospace(true);
            commandWidget->setBlockBackground(codeBackgroundColor());
            commandWidget->setExpanded(m_toolCommandExpanded);
            commandWidget->setText(m_toolCommand);
            connect(commandWidget, &ACPCollapsibleTextWidget::expandedChanged, this, [this](bool expanded) {
                m_toolCommandExpanded = expanded;
                Q_EMIT contentSizeChanged();
            });
            toolLayout->addWidget(commandWidget);
        }

        // Third row: content if available (from tool call updates)
        if (!m_toolContent.isEmpty()) {
            ACPCollapsibleTextWidget *contentWidget = new ACPCollapsibleTextWidget(i18n("Output"), toolWidget);
            contentWidget->setBlockBackground(codeBackgroundColor());
            contentWidget->setExpanded(m_toolContentExpanded);
            contentWidget->setText(m_toolContent);
            connect(contentWidget, &ACPCollapsibleTextWidget::expandedChanged, this, [this](bool expanded) {
                m_toolContentExpanded = expanded;
                Q_EMIT contentSizeChanged();
            });
            toolLayout->addWidget(contentWidget);
        }

        contentLayout->addWidget(toolWidget);
        break;
    }
    case MessageType::ToolCallUpdate: {
        // The status and the identifier are shown in the header, and the type
        // of the message already says that this is an update, so all that is
        // left for the content is the output the update carries
        QWidget *updateWidget = new QWidget(m_contentWidget);
        updateWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        QVBoxLayout *updateLayout = new QVBoxLayout(updateWidget);
        updateLayout->setContentsMargins(4, 2, 4, 2);
        updateLayout->setSpacing(8);

        if (!m_toolContent.isEmpty()) {
            ACPCollapsibleTextWidget *contentWidget = new ACPCollapsibleTextWidget(i18n("Output"), updateWidget);
            contentWidget->setBlockBackground(codeBackgroundColor());
            contentWidget->setExpanded(m_toolContentExpanded);
            contentWidget->setText(m_toolContent);
            connect(contentWidget, &ACPCollapsibleTextWidget::expandedChanged, this, [this](bool expanded) {
                m_toolContentExpanded = expanded;
                Q_EMIT contentSizeChanged();
            });
            updateLayout->addWidget(contentWidget);
        }

        contentLayout->addWidget(updateWidget);
        break;
    }
    case MessageType::Usage: {
        QWidget *usageWidget = new QWidget(m_contentWidget);
        usageWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        QHBoxLayout *usageLayout = new QHBoxLayout(usageWidget);
        usageLayout->setContentsMargins(4, 2, 4, 2);
        usageLayout->setSpacing(12);

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
        permissionLayout->setSpacing(6);

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
            monoFont.setPointSize(monoFont.pointSize() - 1);
            commandLabel->setFont(monoFont);
            // Use alternate background for command display with proper text color
            QPalette cmdPal = commandLabel->palette();
            cmdPal.setColor(QPalette::Base, cmdPal.color(QPalette::AlternateBase));
            cmdPal.setColor(QPalette::WindowText, cmdPal.color(QPalette::Text));
            commandLabel->setPalette(cmdPal);
            commandLabel->setAutoFillBackground(true);
            commandLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            commandLabel->setWordWrap(true);
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
            for (const PermissionOption &option : std::as_const(m_permissionOptions)) {
                QPushButton *button = new QPushButton(option.name, buttonWidget);
                button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                button->setProperty("requestId", QVariant::fromValue(m_requestId));
                button->setProperty("optionId", QVariant::fromValue(option.optionId));

                // Apply styling based on the option kind using palette colors
                QPalette btnPal = button->palette();
                if (option.kind == ACP::permissionKindAllowAlways()) {
                    // Use LinkVisited (typically green/purple) for "always allow"
                    btnPal.setColor(QPalette::ButtonText, btnPal.color(QPalette::LinkVisited));
                } else if (option.kind == ACP::permissionKindRejectAlways()) {
                    // Use BrightText (typically red) for "always reject"
                    btnPal.setColor(QPalette::ButtonText, btnPal.color(QPalette::BrightText));
                } else if (option.kind == ACP::permissionKindAllowOnce()) {
                    // Use Link (typically blue) for "allow once"
                    btnPal.setColor(QPalette::ButtonText, btnPal.color(QPalette::Link));
                } else if (option.kind == ACP::permissionKindRejectOnce()) {
                    // Use Mid (gray) for "reject once"
                    btnPal.setColor(QPalette::ButtonText, btnPal.color(QPalette::Mid));
                }
                button->setPalette(btnPal);

                connect(button, &QPushButton::clicked, this, [this, button]() {
                    // Disable the button to show the action was taken
                    button->setEnabled(false);
                    // Disable all other permission buttons in the same widget
                    QWidget *buttonWidget = button->parentWidget();
                    if (buttonWidget) {
                        const auto l = buttonWidget->findChildren<QPushButton *>(QString(), Qt::FindDirectChildrenOnly);
                        for (auto b : l) {
                            b->setEnabled(false);
                        }
                    }
                    Q_EMIT permissionResponse(button->property("requestId").toJsonValue(), button->property("optionId").toString());
                });
                buttonLayout->addWidget(button);
            }
        } else {
            // Fallback to old behavior with just allow/reject buttons
            QPushButton *allowButton = new QPushButton(i18n("Allow"), buttonWidget);
            allowButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            allowButton->setProperty("requestId", QVariant::fromValue(m_requestId));
            allowButton->setProperty("optionId", QVariant::fromValue(m_permissionAllowOptionId));
            QPalette allowPal = allowButton->palette();
            allowPal.setColor(QPalette::ButtonText, allowPal.color(QPalette::LinkVisited));
            allowButton->setPalette(allowPal);

            QPushButton *rejectButton = new QPushButton(i18n("Reject"), buttonWidget);
            rejectButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            rejectButton->setProperty("requestId", QVariant::fromValue(m_requestId));
            rejectButton->setProperty("optionId", QVariant::fromValue(m_permissionRejectOptionId));
            QPalette rejectPal = rejectButton->palette();
            rejectPal.setColor(QPalette::ButtonText, rejectPal.color(QPalette::BrightText));
            rejectButton->setPalette(rejectPal);

            connect(allowButton, &QPushButton::clicked, this, [this, allowButton, rejectButton]() {
                // Disable both buttons to show the action was taken
                allowButton->setEnabled(false);
                rejectButton->setEnabled(false);
                Q_EMIT permissionResponse(allowButton->property("requestId").toJsonValue(), allowButton->property("optionId").toString());
            });
            connect(rejectButton, &QPushButton::clicked, this, [this, allowButton, rejectButton]() {
                // Disable both buttons to show the action was taken
                allowButton->setEnabled(false);
                rejectButton->setEnabled(false);
                Q_EMIT permissionResponse(rejectButton->property("requestId").toJsonValue(), rejectButton->property("optionId").toString());
            });

            buttonLayout->addWidget(allowButton);
            buttonLayout->addWidget(rejectButton);
        }

        permissionLayout->addWidget(buttonWidget);
        contentLayout->addWidget(permissionWidget);
        break;
    }
    }

    // A widget added to a layout only becomes visible once the posted layout
    // request is delivered, which happens after control returns to the event
    // loop. Until then it is skipped by the layout, so the message reports no
    // heightForWidth and a size hint that fits none of its content. Whoever
    // rebuilt the content measures it right afterwards, so show the new
    // widgets now instead of letting them be measured as if they were absent.
    if (!isHidden()) {
        for (int i = 0; i < contentLayout->count(); ++i) {
            if (QWidget *child = contentLayout->itemAt(i)->widget()) {
                child->show();
            }
        }
    }

    m_typeLabel->setText(getTypeLabel());
    m_timestampLabel->setText(formatTimestamp(m_timestamp));

    // The bubble is painted from the current status, so a tool call that just
    // failed has to repaint its background and border
    update();
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
    case MessageType::Thought:
        return i18n("Thinking");
    case MessageType::Pending:
        return i18n("Working");
    case MessageType::Queued:
        return i18n("Queued");
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
    case MessageType::Error:
        return i18n("Error");
    }
    return QString();
}

#include "moc_acpchatmessagewidget.cpp"
