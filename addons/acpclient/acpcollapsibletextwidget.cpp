/*
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpcollapsibletextwidget.h"

#include <KLocalizedString>

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

/** @brief Preview lines longer than this are shortened */
static constexpr int s_maxPreviewLineLength = 400;

ACPCollapsibleTextWidget::ACPCollapsibleTextWidget(const QString &label, QWidget *parent)
    : QFrame(parent)
    , m_label(label)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    // A border separates this block from the one next to it
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Plain);

    QVBoxLayout *layout = new QVBoxLayout(this);

    // Keep the text off the edge of the block, the same way the code blocks of
    // a message are padded
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(2);

    // Header, naming the block and toggling the preview
    QWidget *header = new QWidget(this);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(6);

    // The name is shown even when the text is too short to be collapsed, so
    // that input and output blocks can always be told apart
    m_nameLabel = new QLabel(m_label, header);
    QFont nameFont = m_nameLabel->font();
    nameFont.setBold(true);
    nameFont.setPointSize(qMax(1, nameFont.pointSize() - 1));
    m_nameLabel->setFont(nameFont);
    m_nameLabel->setVisible(!m_label.isEmpty());
    headerLayout->addWidget(m_nameLabel);

    m_headerButton = new QToolButton(header);
    m_headerButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_headerButton->setAutoRaise(true);
    m_headerButton->setCursor(Qt::PointingHandCursor);
    m_headerButton->setFocusPolicy(Qt::StrongFocus);
    m_headerButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    connect(m_headerButton, &QToolButton::clicked, this, [this]() {
        setExpanded(!m_expanded);
    });
    headerLayout->addWidget(m_headerButton);
    headerLayout->addStretch();

    layout->addWidget(header);

    // The whole text, or the leading lines of the preview
    m_headLabel = new QLabel(this);
    m_headLabel->setWordWrap(true);
    m_headLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout->addWidget(m_headLabel);

    // Separator between the leading and trailing preview lines
    m_moreButton = new QToolButton(this);
    m_moreButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_moreButton->setAutoRaise(true);
    m_moreButton->setCursor(Qt::PointingHandCursor);
    m_moreButton->setFocusPolicy(Qt::NoFocus);
    QFont moreFont = m_moreButton->font();
    moreFont.setItalic(true);
    m_moreButton->setFont(moreFont);
    connect(m_moreButton, &QToolButton::clicked, this, [this]() {
        setExpanded(true);
    });
    layout->addWidget(m_moreButton, 0, Qt::AlignLeft);

    // Trailing lines of the preview
    m_tailLabel = new QLabel(this);
    m_tailLabel->setWordWrap(true);
    m_tailLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout->addWidget(m_tailLabel);

    updateDisplay();
}

void ACPCollapsibleTextWidget::setText(const QString &text)
{
    // Trailing newlines would be displayed as empty preview lines
    QString normalized = text;
    while (normalized.endsWith(QLatin1Char('\n')) || normalized.endsWith(QLatin1Char('\r'))) {
        normalized.chop(1);
    }

    if (m_text == normalized) {
        return;
    }

    m_text = normalized;
    m_lines = m_text.split(QLatin1Char('\n'));
    updateDisplay();
}

QString ACPCollapsibleTextWidget::text() const
{
    return m_text;
}

void ACPCollapsibleTextWidget::setMonospace(bool monospace)
{
    QFont textFont = font();
    if (monospace) {
        textFont = QFont(QStringLiteral("monospace"));
        textFont.setPointSize(textFont.pointSize() - 1);
    }

    m_headLabel->setFont(textFont);
    m_tailLabel->setFont(textFont);
}

void ACPCollapsibleTextWidget::setBlockBackground(const QColor &color)
{
    QPalette blockPalette = palette();
    blockPalette.setColor(QPalette::Window, color);
    setPalette(blockPalette);

    // The labels and buttons of the block are transparent, so filling the
    // frame is enough to color the whole block
    setAutoFillBackground(true);
}

void ACPCollapsibleTextWidget::setCollapsedLineLimit(int lines)
{
    m_collapsedLineLimit = qMax(1, lines);
    updateDisplay();
}

void ACPCollapsibleTextWidget::setContextLineCount(int lines)
{
    m_contextLines = qMax(1, lines);
    updateDisplay();
}

void ACPCollapsibleTextWidget::setCollapseMode(CollapseMode mode)
{
    if (m_collapseMode == mode) {
        return;
    }

    m_collapseMode = mode;
    updateDisplay();
}

bool ACPCollapsibleTextWidget::isCollapsible() const
{
    if (m_text.isEmpty()) {
        return false;
    }

    // Text that is hidden as a whole is worth hiding at any size
    if (m_collapseMode == CollapseMode::HideAll) {
        return true;
    }

    return m_lines.size() > m_collapsedLineLimit || m_text.size() > m_collapsedCharLimit;
}

bool ACPCollapsibleTextWidget::isExpanded() const
{
    return m_expanded;
}

void ACPCollapsibleTextWidget::setExpanded(bool expanded)
{
    if (m_expanded == expanded) {
        return;
    }

    m_expanded = expanded;
    updateDisplay();

    Q_EMIT expandedChanged(m_expanded);
}

int ACPCollapsibleTextWidget::hiddenLineCount() const
{
    if (!isCollapsible()) {
        return 0;
    }

    // Nothing to hide if the preview would show every line anyway
    if (m_lines.size() <= 2 * m_contextLines) {
        return 0;
    }

    return m_lines.size() - 2 * m_contextLines;
}

int ACPCollapsibleTextWidget::hiddenCharacterCount() const
{
    return qMax(0, static_cast<int>(m_text.size()) - m_collapsedCharLimit / 2);
}

QString ACPCollapsibleTextWidget::previewBlock(int from, int count) const
{
    QStringList block;
    for (int i = qMax(0, from); i < from + count && i < m_lines.size(); ++i) {
        QString line = m_lines.at(i);
        if (line.size() > s_maxPreviewLineLength) {
            line = line.left(s_maxPreviewLineLength) + QStringLiteral("…");
        }
        block.append(line);
    }

    return block.join(QLatin1Char('\n'));
}

void ACPCollapsibleTextWidget::updateDisplay()
{
    const bool collapsible = isCollapsible();
    const int lineCount = m_lines.size();

    // Only collapsible text needs a toggle, the name of the block is shown
    // either way so that adjacent blocks stay distinguishable
    m_headerButton->setVisible(collapsible);
    m_headerButton->setArrowType(collapsible && !m_expanded ? Qt::RightArrow : Qt::DownArrow);
    m_headerButton->setText(i18ncp("@action:button size of a text block", "%1 line", "%1 lines", lineCount));
    m_headerButton->setToolTip(m_expanded ? i18nc("@info:tooltip", "Click to collapse the text") : i18nc("@info:tooltip", "Click to show the whole text"));

    // Short text, or text the user expanded, is shown as it is
    if (!collapsible || m_expanded) {
        m_headLabel->setText(m_text);
        m_headLabel->setVisible(true);
        m_moreButton->setVisible(false);
        m_tailLabel->setVisible(false);
        return;
    }

    // Text that is secondary as a whole shows nothing but its header, because
    // a few lines ripped out of the middle of it say nothing useful
    if (m_collapseMode == CollapseMode::HideAll) {
        m_headLabel->setVisible(false);
        m_moreButton->setVisible(false);
        m_tailLabel->setVisible(false);
        return;
    }

    m_headLabel->setVisible(true);

    const int hiddenLines = hiddenLineCount();
    if (hiddenLines > 0) {
        m_headLabel->setText(previewBlock(0, m_contextLines));
        m_tailLabel->setText(previewBlock(lineCount - m_contextLines, m_contextLines));
        m_tailLabel->setVisible(true);
        m_moreButton->setText(i18ncp("@action:button", "… %1 more line …", "… %1 more lines …", hiddenLines));
    } else if (m_text.size() > m_collapsedCharLimit) {
        // Few but very long lines: show the beginning of the text only
        m_headLabel->setText(m_text.left(m_collapsedCharLimit / 2) + QStringLiteral("…"));
        m_tailLabel->setVisible(false);
        m_moreButton->setText(i18ncp("@action:button", "… %1 more character …", "… %1 more characters …", hiddenCharacterCount()));
    } else {
        // Text that is collapsible only because it is secondary, and short
        // enough that the preview would show all of it anyway
        m_headLabel->setText(m_text);
        m_tailLabel->setVisible(false);
        m_moreButton->setVisible(false);
        return;
    }

    m_moreButton->setToolTip(m_headerButton->toolTip());
    m_moreButton->setVisible(true);
}

#include "moc_acpcollapsibletextwidget.cpp"
