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
            return KSyntaxHighlighting::SyntaxHighlighter::highlightBlock(text);
        }
        const int headingLevel = fmt.headingLevel();

        const QList<QTextLayout::FormatRange> textFormats = block.textFormats();
        for (const auto &f : textFormats) {
            if (f.format.isAnchor()) {
                QTextCharFormat charFmt = format(f.start);
                charFmt.setFontUnderline(true);
                // charFmt.setForeground(linkColor);
                setFormat(f.start, f.length, charFmt);
            } else if (headingLevel != 0 && f.format.hasProperty(QTextFormat::FontSizeAdjustment)) {
                QTextCharFormat charFmt = format(f.start);
                charFmt.setProperty(QTextFormat::FontSizeAdjustment, 1);
                setFormat(0, text.size(), charFmt);
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

    TooltipPrivate(QWidget *parent, bool manual)
        : QTextBrowser(parent)
        , m_hl(new TooltipHighlighter(document()))
        , m_manual(manual)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::BypassGraphicsProxyWidget | Qt::ToolTip);
        setAttribute(Qt::WA_DeleteOnClose, true);
        document()->setDocumentMargin(5);
        setFrameStyle(QFrame::Box | QFrame::Raised);
        connect(&m_hideTimer, &QTimer::timeout, this, &TooltipPrivate::hideTooltip);

        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        // doc links
        setOpenExternalLinks(true);

        auto updateColors = [this](KTextEditor::Editor *e) {
            auto theme = e->theme();
            m_hl->setTheme(theme);

            auto pal = palette();
            const QColor bg = theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor);
            pal.setColor(QPalette::Base, bg);
            const QColor normal = theme.textColor(KSyntaxHighlighting::Theme::Normal);
            pal.setColor(QPalette::Text, normal);
            setPalette(pal);

            setFont(KTextEditor::Editor::instance()->font());
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
        const int contentsMarginsHeight = this->contentsMargins().top() + this->contentsMargins().top();
        const int docMargin = 2 * document()->documentMargin();
        const int wMargins = contentsMarginsWidth + docMargin + verticalScrollBar()->height();
        const int hMargins = contentsMarginsHeight + docMargin + horizontalScrollBar()->width();

        // add internal document padding and possible scrollbars to size
        size.setHeight(size.height() + hMargins);
        size.setWidth(size.width() + wMargins);

        // limit the tool tip size; the resize call will respect these limits
        setMaximumSize(m_view->width() / 2.5, m_view->height() / 3);
        resize(size);
    }

    void place(QPoint p)
    {
        const auto offset = QPoint(3, 21);
        p += offset;

        // wayland automatically keeps popups on screen
        if (KWindowSystem::isPlatformWayland()) {
            move(p);
            return;
        }

        // try to get right screen, important: QApplication::screenAt(p) might return nullptr
        // see crash in bug 439804
        const QScreen *screenForTooltip = QApplication::screenAt(p);
        if (!screenForTooltip) {
            screenForTooltip = screen();
        }
        const QRect screen = screenForTooltip->availableGeometry();

        if (p.x() + width() > screen.x() + screen.width())
            p.rx() -= 4 + width();
        if (p.y() + this->height() > screen.y() + screen.height())
            p.ry() -= 24 + this->height();
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
            return QTextBrowser::mouseMoveEvent(event);
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
};

void KateTooltip::show(size_t instanceId, const QString &text, TextHintMarkupKind kind, QPoint pos, KTextEditor::View *v, bool manual)
{
    if (!v || !v->document()) {
        return;
    }

    static QPointer<TooltipPrivate> tooltip = nullptr;
    if (tooltip && tooltip->isVisible()) {
        if (text.isEmpty() || text.trimmed().isEmpty()) {
            tooltip->remove(instanceId);
            return;
        }

        tooltip->upsert(instanceId, text, kind);
        return;
    }
    delete tooltip;

    if (text.isEmpty() || text.trimmed().isEmpty()) {
        return;
    }

    tooltip = new TooltipPrivate(v, manual);
    tooltip->setView(v);
    tooltip->upsert(instanceId, text, kind);
    tooltip->place(pos);
    tooltip->show();
}
