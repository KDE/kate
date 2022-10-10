/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "diffeditor.h"
#include "difflinenumarea.h"
#include "diffwidget.h"
#include "ktexteditor_utils.h"

#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QTextBlock>

#include <KLocalizedString>
#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/State>
#include <KTextEditor/Editor>

DiffSyntaxHighlighter::DiffSyntaxHighlighter(QTextDocument *parent, DiffWidget *diffWidget)
    : KSyntaxHighlighting::SyntaxHighlighter(parent)
    , m_diffWidget(diffWidget)
{
}

void DiffSyntaxHighlighter::applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format)
{
    if (format.textStyle() == KSyntaxHighlighting::Theme::TextStyle::Error) {
        return;
    }
    KSyntaxHighlighting::SyntaxHighlighter::applyFormat(offset, length, format);
}

void DiffSyntaxHighlighter::highlightBlock(const QString &text)
{
    // Delete user data i.e., the stored state in the block
    // when we encounter a hunk to avoid issues like everything
    // is commented because previous hunk ended with an unclosed
    // comment block
    // do this only if not anyways first block, there user data is not existing
    if (currentBlock().position() > 0 && m_diffWidget->isHunk(currentBlock().blockNumber())) {
        // ownership of the data is in the block, just reset it by assigning a new dummy data
        currentBlock().previous().setUserData(new QTextBlockUserData);
    }
    KSyntaxHighlighting::SyntaxHighlighter::highlightBlock(text);
}

DiffEditor::DiffEditor(DiffParams::Flags f, QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumArea(new LineNumArea(this))
    , m_diffWidget(static_cast<DiffWidget *>(parent))
    , m_flags(f)
{
    setFrameStyle(QFrame::NoFrame);

    auto updateEditorColors = [this](KTextEditor::Editor *e) {
        if (!e)
            return;
        using namespace KSyntaxHighlighting;
        auto theme = e->theme();
        auto bg = QColor::fromRgba(theme.editorColor(Theme::EditorColorRole::BackgroundColor));
        auto fg = QColor::fromRgba(theme.textColor(Theme::TextStyle::Normal));
        auto sel = QColor::fromRgba(theme.editorColor(Theme::EditorColorRole::TextSelection));
        hunkSeparatorColor = QColor::fromRgba(theme.textColor(Theme::TextStyle::Keyword));
        auto pal = palette();
        pal.setColor(QPalette::Base, bg);
        pal.setColor(QPalette::Text, fg);
        // set a small alpha to be able to see the red/green bg
        // with selection
        if (fg.alpha() == 255) {
            sel.setAlphaF(0.8);
        }
        pal.setColor(QPalette::Highlight, sel);
        pal.setColor(QPalette::HighlightedText, fg);
        setFont(Utils::editorFont());
        setPalette(pal);

        updateDiffColors(bg.lightness() < 127);
    };
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, updateEditorColors);
    updateEditorColors(KTextEditor::Editor::instance());

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this] {
        m_lineNumArea->update();
    });
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, [this] {
        m_lineNumArea->update();
    });
    connect(document(), &QTextDocument::blockCountChanged, this, &DiffEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &DiffEditor::updateLineNumberArea);

    setReadOnly(true);

    m_timeLine.setDuration(1000);
    m_timeLine.setFrameRange(0, 250);
    connect(&m_timeLine, &QTimeLine::frameChanged, this, [this](int) {
        if (!m_animateTextRect.isNull()) {
            viewport()->update(m_animateTextRect);
        }
    });
    connect(&m_timeLine, &QTimeLine::finished, this, [this] {
        auto r = m_animateTextRect;
        m_animateTextRect = QRect();
        viewport()->update(r);
    });
}

void DiffEditor::updateDiffColors(bool darkMode)
{
    red1 = darkMode ? QColor(Qt::red).darker(120) : QColor(Qt::red).lighter(140);
    red1.setAlphaF(0.1);
    green1 = darkMode ? QColor(Qt::green).darker(140) : QColor(Qt::green).lighter(140);
    green1.setAlphaF(0.1);

    red2 = darkMode ? QColor(Qt::red).darker(80) : QColor(Qt::darkRed).lighter(120);
    red2.setAlphaF(0.20);
    green2 = darkMode ? QColor(Qt::green).darker(110) : QColor(Qt::darkGreen).lighter(120);
    green2.setAlphaF(0.20);
}

void DiffEditor::scrollToBlock(int block)
{
    if (m_timeLine.state() == QTimeLine::Running) {
        m_timeLine.stop();
        auto r = m_animateTextRect;
        m_animateTextRect = QRect();
        viewport()->update(r);
    }
    int lineNo = 0;
    for (int i = 0; i < block; ++i) {
        lineNo += document()->findBlockByNumber(i).lineCount();
    }
    verticalScrollBar()->setValue(lineNo);

    QTextBlock b = document()->findBlockByNumber(block);
    m_animateTextRect = blockBoundingGeometry(b).translated(contentOffset()).toRect();
    m_timeLine.start();
}

void DiffEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    updateLineNumAreaGeometry();
}

KTextEditor::Range DiffEditor::selectionRange() const
{
    const auto cursor = textCursor();
    if (!cursor.hasSelection())
        return KTextEditor::Range::invalid();

    QTextCursor start = cursor;
    start.setPosition(qMin(cursor.selectionStart(), cursor.selectionEnd()));
    QTextCursor end = cursor;
    end.setPosition(qMax(cursor.selectionStart(), cursor.selectionEnd()));

    const int startLine = start.blockNumber();
    const int endLine = end.blockNumber();
    const int startColumn = start.selectionStart() - start.block().position();
    const int endColumn = end.selectionEnd() - end.block().position();
    return {startLine, startColumn, endLine, endColumn};
}

void DiffEditor::contextMenuEvent(QContextMenuEvent *e)
{
    // Follow KTextEditor behaviour
    if (!textCursor().hasSelection()) {
        setTextCursor(cursorForPosition(e->pos()));
    }

    auto menu = createStandardContextMenu();
    QAction *before = nullptr;
    if (!menu->actions().isEmpty())
        before = menu->actions().constFirst();

    {
        auto a = new QAction(i18n("Change Style"));
        auto styleMenu = new QMenu(this);
        styleMenu->addAction(i18n("Side By Side"), this, [this] {
            Q_EMIT switchStyle(SideBySide);
        });
        styleMenu->addAction(i18n("Unified"), this, [this] {
            Q_EMIT switchStyle(Unified);
        });
        styleMenu->addAction(i18n("Raw"), this, [this] {
            Q_EMIT switchStyle(Raw);
        });
        a->setMenu(styleMenu);
        menu->insertAction(before, a);
    }

    addStageUnstageDiscardActions(menu);

    menu->exec(viewport()->mapToGlobal(e->pos()));
}

void DiffEditor::addStageUnstageDiscardActions(QMenu *menu)
{
    const auto selection = selectionRange();
    const int lineCount = !selection.isValid() ? 1 : selection.numberOfLines() + 1;

    int startLine = textCursor().blockNumber();
    int endLine = startLine;
    if (selection.isValid()) {
        startLine = selection.start().line();
        endLine = selection.end().line();
    }

    QAction *before = nullptr;
    if (!menu->actions().isEmpty())
        before = menu->actions().constFirst();

    if (m_flags.testFlag(DiffParams::Flag::ShowStage)) {
        auto a = new QAction(i18np("Stage Line", "Stage Lines", lineCount));
        connect(a, &QAction::triggered, this, [=] {
            Q_EMIT actionTriggered(this, startLine, endLine, (int)Line, DiffParams::Flag::ShowStage);
        });
        menu->insertAction(before, a);
        a = new QAction(i18n("Stage Hunk"));
        connect(a, &QAction::triggered, this, [=] {
            Q_EMIT actionTriggered(this, startLine, endLine, (int)Hunk, DiffParams::Flag::ShowStage);
        });
        menu->insertAction(before, a);
    }
    if (m_flags.testFlag(DiffParams::Flag::ShowDiscard)) {
        auto a = new QAction(i18np("Discard Line", "Discard Lines", lineCount));
        connect(a, &QAction::triggered, this, [=] {
            Q_EMIT actionTriggered(this, startLine, endLine, (int)Line, DiffParams::Flag::ShowDiscard);
        });
        menu->insertAction(before, a);
        a = new QAction(i18n("Discard Hunk"));
        connect(a, &QAction::triggered, this, [=] {
            Q_EMIT actionTriggered(this, startLine, endLine, (int)Hunk, DiffParams::Flag::ShowDiscard);
        });
        menu->insertAction(before, a);
    }
    if (m_flags.testFlag(DiffParams::Flag::ShowUnstage)) {
        auto a = new QAction(i18np("Unstage Line", "Unstage Lines", lineCount));
        connect(a, &QAction::triggered, this, [=] {
            Q_EMIT actionTriggered(this, startLine, endLine, (int)Line, DiffParams::Flag::ShowUnstage);
        });
        menu->insertAction(before, a);
        a = new QAction(i18n("Unstage Hunk"));
        connect(a, &QAction::triggered, this, [=] {
            Q_EMIT actionTriggered(this, startLine, endLine, (int)Hunk, DiffParams::Flag::ShowUnstage);
        });
        menu->insertAction(before, a);
    }
}

void DiffEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumArea->scroll(0, dy);
    else
        m_lineNumArea->update(0, rect.y(), m_lineNumArea->sizeHint().width(), rect.height());

    updateLineNumAreaGeometry();

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void DiffEditor::updateLineNumAreaGeometry()
{
    const auto contentsRect = this->contentsRect();
    const QRect newGeometry = {contentsRect.left(), contentsRect.top(), m_lineNumArea->sizeHint().width(), contentsRect.height()};
    auto oldGeometry = m_lineNumArea->geometry();
    if (newGeometry != oldGeometry) {
        m_lineNumArea->setGeometry(newGeometry);
    }
}

void DiffEditor::updateLineNumberAreaWidth(int)
{
    QSignalBlocker blocker(this);
    const auto oldMargins = viewportMargins();
    const int width = m_lineNumArea->sizeHint().width();
    const auto newMargins = QMargins{width, oldMargins.top(), oldMargins.right(), oldMargins.bottom()};

    if (newMargins != oldMargins) {
        setViewportMargins(newMargins);
    }
}

void DiffEditor::paintEvent(QPaintEvent *e)
{
    QPainter p(viewport());
    QPointF offset(contentOffset());
    QTextBlock block = firstVisibleBlock();
    const auto viewportRect = viewport()->rect();

    while (block.isValid()) {
        const QRectF r = blockBoundingRect(block).translated(offset);
        auto layout = block.layout();

        auto hl = highlightingForLine(block.blockNumber());
        if (block.isVisible() && hl && layout) {
            const auto changes = hl->changes;
            for (auto c : changes) {
                // full line background is colored
                p.fillRect(r, hl->added ? green1 : red1);
                if (c.len == Change::FullBlock) {
                    continue;
                }

                QTextLine sl = layout->lineForTextPosition(c.pos);
                QTextLine el = layout->lineForTextPosition(c.pos + c.len);
                // color any word diffs
                if (sl.isValid() && sl.lineNumber() == el.lineNumber()) {
                    int sx = sl.cursorToX(c.pos);
                    int ex = el.cursorToX(c.pos + c.len);
                    QRectF r = sl.naturalTextRect();
                    r.setLeft(sx);
                    r.setRight(ex);
                    r.moveTop(offset.y() + (sl.height() * sl.lineNumber()));
                    p.fillRect(r, hl->added ? green2 : red2);
                } else {
                    QPainterPath path;
                    int i = sl.lineNumber() + 1;
                    int end = el.lineNumber();
                    QRectF rect = sl.naturalTextRect();
                    rect.setLeft(sl.cursorToX(c.pos));
                    rect.moveTop(offset.y() + (sl.height() * sl.lineNumber()));
                    path.addRect(rect);
                    for (; i <= end; ++i) {
                        auto line = layout->lineAt(i);
                        rect = line.naturalTextRect();
                        rect.moveTop(offset.y() + (line.height() * line.lineNumber()));
                        if (i == end) {
                            rect.setRight(el.cursorToX(c.pos + c.len));
                        }
                        path.addRect(rect);
                    }
                    p.fillPath(path, hl->added ? green2 : red2);
                }
            }
        }

        if (isHunkLine(block.blockNumber())) {
            p.save();
            QPen pen;
            pen.setColor(hunkSeparatorColor);
            pen.setWidthF(1.1);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawLine(r.topLeft(), r.topRight());
            p.drawLine(r.bottomLeft(), r.bottomRight());
            p.restore();
        }

        if (m_diffWidget->isFileNameLine(block.blockNumber())) {
            p.save();
            QPen pen;
            pen.setColor(hunkSeparatorColor);
            pen.setWidthF(1.1);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            auto rCopy = r;
            rCopy.setRight(block.layout()->lineAt(0).naturalTextRect().right() + 4);
            rCopy.setLeft(rCopy.left() - 2);
            p.drawRect(rCopy);
            p.restore();
        }

        if (m_animateTextRect.contains(offset.toPoint())) {
            QColor c(Qt::red);
            c.setAlpha(m_timeLine.currentFrame());
            p.fillRect(m_animateTextRect, c);
        }

        offset.ry() += r.height();
        if (offset.y() > viewportRect.height()) {
            break;
        }
        block = block.next();
    }

    QPlainTextEdit::paintEvent(e);
}

const LineHighlight *DiffEditor::highlightingForLine(int line)
{
    auto it = std::find_if(m_data.cbegin(), m_data.cend(), [line](LineHighlight hl) {
        return hl.line == line;
    });
    return it == m_data.cend() ? nullptr : &(*it);
}

void DiffEditor::setLineNumberData(QVector<int> lineNosA, QVector<int> lineNosB, int maxLineNum)
{
    m_lineNumArea->setLineNumData(std::move(lineNosA), std::move(lineNosB));
    m_lineNumArea->setMaxLineNum(maxLineNum);
    updateLineNumberAreaWidth(0);
}

DiffEditor::State DiffEditor::saveState() const
{
    return {verticalScrollBar()->value(), textCursor().position()};
}

void DiffEditor::restoreState(State s)
{
    if (document() && document()->isEmpty()) {
        return;
    }

    verticalScrollBar()->setValue(qMax(0, s.scrollValue));
    auto cursor = textCursor();
    cursor.setPosition(qMin(cursor.document()->characterCount(), s.cursorPosition));
    setTextCursor(cursor);
}

bool DiffEditor::isHunkLine(int line) const
{
    return m_diffWidget->isHunk(line);
}

bool DiffEditor::isHunkFolded(int blockNumber)
{
    Q_ASSERT(isHunkLine(blockNumber));
    const auto block = document()->findBlockByNumber(blockNumber).next();
    return block.isValid() && !block.isVisible();
}

void DiffEditor::toggleFoldHunk(int blockNumber)
{
    Q_ASSERT(isHunkLine(blockNumber));
    int count = m_diffWidget->hunkLineCount(blockNumber);
    if (count == 0) {
        return;
    }
    if (count == -1) {
        count = blockCount() - blockNumber;
    }
    if (count <= 0) {
        return;
    }

    auto block = document()->findBlockByNumber(blockNumber).next();
    int i = 0;
    bool visible = !block.isVisible();
    while (true) {
        i++;
        if (i == count || !block.isValid()) {
            break;
        }
        block.setVisible(visible);
        block = block.next();
    }

    viewport()->update();
    m_lineNumArea->update();
}
