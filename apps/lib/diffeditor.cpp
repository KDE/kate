#include "diffeditor.h"
#include "difflinenumarea.h"
#include "ktexteditor_utils.h"

#include <QPainter>
#include <QPainterPath>
#include <QTextBlock>

DiffEditor::DiffEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumArea(new LineNumArea(this))
{
    setFont(Utils::editorFont());
    red1 = QColor("#c87872");
    red1.setAlphaF(0.2);
    green1 = QColor("#678528");
    green1.setAlphaF(0.2);

    auto c = QColor(254, 147, 140);
    c.setAlphaF(0.1);
    red2 = c;

    c = QColor(166, 226, 46);
    c.setAlphaF(0.1);
    green2 = c;

    setFrameStyle(QFrame::NoFrame);

    auto updateEditorColors = [this](KTextEditor::Editor *e) {
        if (!e)
            return;
        auto theme = e->theme();
        auto bg = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::BackgroundColor));
        auto fg = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::TextStyle::Normal));
        auto sel = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::TextSelection));
        auto pal = palette();
        pal.setColor(QPalette::Base, bg);
        pal.setColor(QPalette::Text, fg);
        pal.setColor(QPalette::Highlight, sel);
        pal.setColor(QPalette::HighlightedText, fg);
        setPalette(pal);
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
}

void DiffEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    updateLineNumAreaGeometry();
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
    bool textPainted = false;
    if (!getPaintContext().selections.isEmpty()) {
        QPlainTextEdit::paintEvent(e);
        textPainted = true;
    }

    QPainter p(viewport());
    QPointF offset(contentOffset());
    QTextBlock block = firstVisibleBlock();
    const auto viewportRect = viewport()->rect();

    while (block.isValid()) {
        QRectF r = blockBoundingRect(block).translated(offset);
        auto layout = block.layout();

        auto hl = dataForLine(block.blockNumber());
        if (hl && layout) {
            const auto changes = hl->changes;
            for (auto c : changes) {
                // full line background is colored
                p.fillRect(r, hl->added ? green1 : red1);
                if (c.len >= block.text().length()) {
                    continue;
                }
                //                 qDebug() << "..." << c.len << block.text().length();
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

        if (block.text().startsWith(QStringLiteral("@@ "))) {
            p.save();
            p.setPen(Qt::red);
            p.setBrush(Qt::NoBrush);
            QRectF copy = r;
            copy.setRight(copy.right() - 1);
            p.drawRect(copy);
            p.restore();
        }

        offset.ry() += r.height();
        if (offset.y() > viewportRect.height()) {
            break;
        }
        block = block.next();
    }

    if (!textPainted) {
        QPlainTextEdit::paintEvent(e);
    }
}

const LineHighlight *DiffEditor::dataForLine(int line)
{
    auto it = std::find_if(m_data.cbegin(), m_data.cend(), [line](LineHighlight hl) {
        return hl.line == line;
    });
    return it == m_data.cend() ? nullptr : &(*it);
}

void DiffEditor::setLineNumberData(QVector<int> data, int maxLineNum)
{
    m_lineNumArea->setLineNumData(std::move(data));
    m_lineNumArea->setMaxLineNum(maxLineNum);
    updateLineNumberAreaWidth(0);
}
