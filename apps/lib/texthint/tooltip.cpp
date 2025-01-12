/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "tooltip.h"
#include "hintstate.h"

#include <QApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPointer>
#include <QScreen>
#include <QScrollBar>
#include <QString>
#include <QTextBrowser>
#include <QTimer>

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>
#include <KWindowSystem>
#include <QMenu>
#include <QScopedValueRollback>

class TooltipHighlighter final : public KSyntaxHighlighting::SyntaxHighlighter
{
public:
    using KSyntaxHighlighting::SyntaxHighlighter::SyntaxHighlighter;

    void highlightBlock(const QString &text) override
    {
        QTextBlock block = currentBlock();
        QTextBlockFormat fmt = block.blockFormat();
        if (fmt.property(QTextFormat::BlockCodeLanguage).isValid()) {
            // highlight blocks marked with BlockCodeLanguage format
            KSyntaxHighlighting::SyntaxHighlighter::highlightBlock(text);
            return;
        }

        const QList<QTextLayout::FormatRange> textFormats = block.textFormats();
        for (const auto &f : textFormats) {
            if (f.format.isAnchor()) {
                QTextCharFormat charFmt = format(f.start);
                charFmt.setFontUnderline(true);
                // charFmt.setForeground(linkColor);
                setFormat(f.start, f.length, charFmt);
            } else if (f.format.fontFixedPitch()) {
                QTextCharFormat charFmt = format(f.start);
                charFmt.setBackground(m_inlineCodeSpanColor);
                charFmt.setFont(m_editorFont);
                setFormat(f.start, f.length, charFmt);
            }
        }
    }

    void applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format) override
    {
        if (format.textStyle() == KSyntaxHighlighting::Theme::TextStyle::Error) {
            return;
        }
        KSyntaxHighlighting::SyntaxHighlighter::applyFormat(offset, length, format);
    }

    QBrush m_inlineCodeSpanColor;
    QFont m_editorFont;
};

class TooltipPrivate : public QTextBrowser
{
public:
    void setView(KTextEditor::View *view)
    {
        // view changed?
        // => update definition
        // => update font
        if (view != m_view) {
            if (m_view && m_view->focusProxy()) {
                m_view->focusProxy()->removeEventFilter(this);
            }

            m_view = view;

            m_hl->setDefinition(KTextEditor::Editor::instance()->repository().definitionForFileName(m_view->document()->url().toString()));

            if (m_view && m_view->focusProxy()) {
                m_view->focusProxy()->installEventFilter(this);
            }
        }
    }

    TooltipPrivate(QWidget *parent, bool manual, KTextEditor::Range wordRange)
        : QTextBrowser(parent)
        , m_hl(new TooltipHighlighter(document()))
        , m_manual(manual)
        , m_hoveredWordRange(wordRange)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::BypassGraphicsProxyWidget | Qt::ToolTip);
        setAttribute(Qt::WA_DeleteOnClose, true);
        document()->setDocumentMargin(5);
        document()->setTextWidth(-1);
        setFrameStyle(QFrame::Box | QFrame::Raised);
        connect(&m_hideTimer, &QTimer::timeout, this, &TooltipPrivate::hideTooltip);
        setWordWrapMode(QTextOption::WordWrap);

        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        // doc links
        setOpenExternalLinks(true);

        auto updateColors = [this](KTextEditor::Editor *e) {
            auto theme = e->theme();
            m_hl->setTheme(theme);

            auto pal = palette();
            const QColor bg = theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor);
            const QColor normal = theme.textColor(KSyntaxHighlighting::Theme::Normal);
            const QColor separator = bg.lightness() < 127 ? normal.darker() : normal.lighter(180);
            // Frame color
            pal.setColor(QPalette::Dark, separator);
            pal.setColor(QPalette::Light, separator);

            pal.setColor(QPalette::Base, bg);
            pal.setColor(QPalette::Text, normal);
            setPalette(pal);

            if (bg.lightness() < 127) {
                m_hl->m_inlineCodeSpanColor = bg.lighter();
            } else {
                m_hl->m_inlineCodeSpanColor = bg.darker(120);
            }

            auto newFont = KTextEditor::Editor::instance()->font();
            newFont.setPointSize(font().pointSize());
            setFont(newFont);
            m_hl->m_editorFont = newFont;
        };
        updateColors(KTextEditor::Editor::instance());
        connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, updateColors);
    }

    double distance(QPoint p1, QPoint p2)
    {
        auto dx = (p1.x() - p2.x());
        auto dy = (p1.y() - p2.y());
        return sqrt((dx * dx) + (dy * dy));
    }

    bool eventFilter(QObject *o, QEvent *e) override
    {
        switch (e->type()) {
        // only consider KeyPress
        // a key release might get triggered by the trail of a shortcut key activation
        case QEvent::KeyPress:
            hideTooltip();
            break;
        case QEvent::WindowActivate:
        case QEvent::WindowDeactivate:
        case QEvent::FocusOut:
        case QEvent::FocusIn:
            if (!inContextMenu && !m_view->hasFocus()) {
                hideTooltip();
            }
            break;
        case QEvent::MouseMove: {
            if (o == verticalScrollBar() || o == horizontalScrollBar()) {
                return false;
            }

            // initialize distance between current mouse pos and top left corner of the tooltip
            if (prevDistance == 0.0) {
                auto pt = static_cast<QSinglePointEvent *>(e)->globalPosition().toPoint();
                prevDistance = distance(pt, mapToGlobal(rect().topLeft()));
                return false;
            }

            auto pt = static_cast<QSinglePointEvent *>(e)->globalPosition().toPoint();
            auto cursor = m_view->coordinatesToCursor(m_view->mapFromGlobal(pt));
            // we are hovering over the word for which this tooltip exists, dont hide
            if (m_hoveredWordRange == m_view->document()->wordRangeAt(cursor)) {
                return false;
            }

            auto newDistance = distance(pt, mapToGlobal(rect().topLeft()));
            auto pos = mapFromGlobal(static_cast<QSinglePointEvent *>(e)->globalPosition()).toPoint();
            if (!m_manual && !hasFocus() && !rect().contains(pos)) {
                if (newDistance > prevDistance) {
                    prevDistance = newDistance;
                    hideTooltipWithDelay();
                } else {
                    prevDistance = newDistance;
                }
            }
        } break;
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::Wheel: {
            auto pos = mapFromGlobal(static_cast<QSinglePointEvent *>(e)->globalPosition()).toPoint();
            if (!rect().contains(pos)) {
                hideTooltip();
            }
        } break;
        default:
            break;
        }
        return false;
    }

    void hideTooltip()
    {
        deleteLater();
    }

    void hideTooltipWithDelay()
    {
        m_hideTimer.start(100);
    }

    void resizeTip()
    {
        // get a natural size for the document
        document()->adjustSize();
        QSize size = document()->size().toSize();

        const int contentsMarginsWidth = this->contentsMargins().left() + this->contentsMargins().right();
        const int contentsMarginsHeight = this->contentsMargins().top() + this->contentsMargins().bottom();
        const int docMargin = 2 * document()->documentMargin();
        int wMargins = contentsMarginsWidth + docMargin + verticalScrollBar()->width();
        int hMargins = contentsMarginsHeight;

        // add internal document padding and possible scrollbars to size
        size.setHeight(size.height() + hMargins);
        size.setWidth(size.width() + wMargins);

        // limit the tool tip size; the resize call will respect these limits

        const auto fullWith = m_view->window() ? m_view->window()->width() : m_view->width();
        setMaximumSize(fullWith / 2.5, m_view->height() / 3);
        resize(size);
    }

    void place(QPoint p)
    {
        // snap the tooltip to the word we are hovering
        auto wordRange = m_hoveredWordRange;
        QPoint wordStart = m_view->mapToGlobal(m_view->cursorToCoordinate(wordRange.start()));
        QPoint wordEnd = m_view->mapToGlobal(m_view->cursorToCoordinate(wordRange.end()));

        // FIXME: need to account for line height multiplier but ktexteditor doesn't expose that apparently :/
        auto fontConfig = m_view->configValue(QString::fromUtf8("font"));
        int lineHeight = QFontMetrics(fontConfig.value<QFont>()).height();

        // try to get right screen, important: QApplication::screenAt(p) might return nullptr
        // see crash in bug 439804
        const QScreen *screenForTooltip = QApplication::screenAt(p);
        if (!screenForTooltip) {
            screenForTooltip = screen();
        }
        QRect screen = screenForTooltip->availableGeometry();

        // on wayland the tooltip can't overlap panels so it will get shifted and overlap the word we are hovering
        // so by clipping it to the window it wont overlap the panel (assuming the kate window is not overlapping the panel) and get shifted
        if (KWindowSystem::isPlatformWayland()) {
            screen = m_view->window()->geometry();
        }

        p = QPoint(wordStart.x(), wordStart.y() + lineHeight);

        if (p.x() + width() > screen.x() + screen.width())
            p.setX(wordEnd.x() - width());
        if (p.y() + this->height() > screen.y() + screen.height())
            p.ry() -= lineHeight + this->height();
        if (p.y() < screen.y())
            p.setY(screen.y());
        if (p.x() + this->width() > screen.x() + screen.width())
            p.setX(screen.x() + screen.width() - this->width());
        if (p.x() < screen.x())
            p.setX(screen.x());
        if (p.y() + this->height() > screen.y() + screen.height())
            p.setY(screen.y() + screen.height() - this->height());

        this->move(p);
    }

    void upsert(size_t instanceId, const QString &text, TextHintMarkupKind kind)
    {
        m_hintState.upsert(instanceId, text, kind);
        triggerChange();
    }

    void remove(size_t instanceId)
    {
        m_hintState.remove(instanceId);
        triggerChange();
    }

protected:
    void enterEvent(QEnterEvent *event) override
    {
        m_hideTimer.stop();
        QTextBrowser::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        if (!m_hideTimer.isActive() && !inContextMenu) {
            hideTooltip();
        }
        QTextBrowser::leaveEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        auto pos = event->pos();
        if (rect().contains(pos)) {
            QTextBrowser::mouseMoveEvent(event);
            return;
        }
    }

    void contextMenuEvent(QContextMenuEvent *e) override
    {
        QScopedValueRollback r(inContextMenu, true);
        auto m = createStandardContextMenu();
        m->setParent(this);
        e->accept();
        m->exec(mapToGlobal(e->pos()));

        // if cursor has left the widget -> hide
        if (!rect().contains(mapFromGlobal(QCursor::pos()))) {
            hideTooltip();
        }
    }

private:
    void triggerChange()
    {
        m_hintState.render([this](const HintState::Hint &data) {
            const auto &[text, kind] = data;
            if (kind == TextHintMarkupKind::PlainText) {
                setPlainText(text);
            } else {
                setMarkdown(text);
            }

            auto block = document()->firstBlock();
            for (int i = 0; i < document()->blockCount(); ++i) {
                auto bfmt = block.blockFormat();
                // Fix some things for code blocks in markdown
                if (bfmt.hasProperty(QTextFormat::BlockCodeLanguage)) {
                    QTextCursor c(block);
                    // allow word wrap
                    bfmt.setNonBreakableLines(false);
                    // fix the font, use our own mono font
                    c.select(QTextCursor::BlockUnderCursor);
                    auto cfmt = block.charFormat();
                    cfmt.setFont(this->font());
                    c.mergeBlockFormat(bfmt);
                    c.setCharFormat(cfmt);
                } else if (bfmt.headingLevel() != 0) {
                    // Make all headings H3
                    QTextCursor c(block);
                    bfmt.setHeadingLevel(3);
                    c.select(QTextCursor::BlockUnderCursor);
                    QTextCharFormat cfmt = block.charFormat();
                    cfmt.setProperty(QTextFormat::FontSizeAdjustment, 0);
                    cfmt.setFontWeight(QFont::Bold);
                    c.mergeBlockFormat(bfmt);
                    c.setCharFormat(cfmt);
                }
                block = block.next();
            }

            resizeTip();
        });
    }

    bool inContextMenu = false;
    QPointer<KTextEditor::View> m_view;
    QTimer m_hideTimer;
    TooltipHighlighter *m_hl;
    bool m_manual;
    HintState m_hintState;
    double prevDistance = 0.0;
    const KTextEditor::Range m_hoveredWordRange;
};

QObject *KateTooltip::show(size_t instanceId,
                           const QString &text,
                           TextHintMarkupKind kind,
                           QPoint pos,
                           KTextEditor::View *v,
                           bool manual,
                           KTextEditor::Range hoveredRange)
{
    if (!v || !v->document()) {
        return nullptr;
    }

    static QPointer<TooltipPrivate> tooltip = nullptr;
    if (tooltip && tooltip->isVisible()) {
        if (text.isEmpty() || text.trimmed().isEmpty()) {
            tooltip->remove(instanceId);
            return tooltip;
        }

        tooltip->upsert(instanceId, text, kind);
        return tooltip;
    }
    delete tooltip;

    if (text.isEmpty() || text.trimmed().isEmpty()) {
        return tooltip;
    }

    tooltip = new TooltipPrivate(v, manual, hoveredRange);
    tooltip->setView(v);
    tooltip->upsert(instanceId, text, kind);
    tooltip->place(pos);
    tooltip->show();
    return tooltip;
}
