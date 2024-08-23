/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "tooltip.h"

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
        const auto fmt = currentBlock().blockFormat();
        if (fmt.property(QTextFormat::BlockCodeLanguage).isValid()) {
            // highlight blocks marked with BlockCodeLanguage format
            return KSyntaxHighlighting::SyntaxHighlighter::highlightBlock(text);
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
    void setTooltipText(const QString &text, TextHintMarkupKind kind)
    {
        if (text.isEmpty())
            return;

        m_kind = kind;
        // we have to do this to handle soft line
        if (kind == TextHintMarkupKind::PlainText) {
            setPlainText(text);
        } else {
            QString htext = text;
            setMarkdown(htext);
        }
        resizeTip(text);
    }

    void appendMarkdown(const QString &text)
    {
        auto md = toMarkdown();
        // hbreak
        md += QStringLiteral("\n----\n");
        md += text;
        setMarkdown(md);
    }

    void appendTooltipText(const QString &text, TextHintMarkupKind kind)
    {
        auto cursor = textCursor();
        cursor.movePosition(QTextCursor::End);
        if (m_kind == TextHintMarkupKind::PlainText && kind == TextHintMarkupKind::PlainText) {
            cursor.insertText(QStringLiteral("\n"));
            cursor.insertText(text);
        } else if (m_kind == TextHintMarkupKind::MarkDown && kind == TextHintMarkupKind::MarkDown) {
            appendMarkdown(text);
        } else if (m_kind == TextHintMarkupKind::PlainText && kind == TextHintMarkupKind::MarkDown) {
            appendMarkdown(text);
        } else if (m_kind == TextHintMarkupKind::MarkDown && kind == TextHintMarkupKind::PlainText) {
            appendMarkdown(text);
        }

        // resize if too small
        if (height() < 300) {
            resize(width(), height() + 200);
        }
    }

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

            hl->setDefinition(KTextEditor::Editor::instance()->repository().definitionForFileName(m_view->document()->url().toString()));

            if (m_view && m_view->focusProxy()) {
                m_view->focusProxy()->installEventFilter(this);
            }
        }
    }

    TooltipPrivate(QWidget *parent, bool manual)
        : QTextBrowser(parent)
        , hl(new TooltipHighlighter(document()))
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
            hl->setTheme(theme);

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
            auto pos = mapFromGlobal(static_cast<QSinglePointEvent *>(e)->globalPosition()).toPoint();
            if (!m_manual && !hasFocus() && !rect().contains(pos)) {
                hideTooltipWithDelay();
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

    void resizeTip(const QString &text)
    {
        QFontMetrics fm(font());
        QSize size = fm.size(0, text);

        const int height = fm.lineSpacing() * (document()->lineCount());

        size.setHeight(std::min(height, m_view->height() / 3));
        size.setWidth(std::min<int>(size.width(), m_view->width() / 2.5));

        const int contentsMarginsWidth = this->contentsMargins().left() + this->contentsMargins().right();
        const int contentsMarginsHeight = this->contentsMargins().top() + this->contentsMargins().top();
        const int docMargin = 2 * document()->documentMargin();
        const int wMargins = contentsMarginsWidth + docMargin + verticalScrollBar()->height();
        const int hMargins = contentsMarginsHeight + docMargin + horizontalScrollBar()->height();

        size.setWidth(size.width() + wMargins);
        size.setHeight(size.height() + hMargins);
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
    bool inContextMenu = false;
    QPointer<KTextEditor::View> m_view;
    QTimer m_hideTimer;
    KSyntaxHighlighting::SyntaxHighlighter *hl;
    bool m_manual;
    TextHintMarkupKind m_kind = TextHintMarkupKind::PlainText;
};

void KateTooltip::show(const QString &text, TextHintMarkupKind kind, QPoint pos, KTextEditor::View *v, bool manual)
{
    if (text.isEmpty())
        return;

    if (!v || !v->document()) {
        return;
    }

    static QPointer<TooltipPrivate> tooltip = nullptr;
    if (tooltip && tooltip->isVisible()) {
        tooltip->appendTooltipText(text, kind);
        return;
    }
    delete tooltip;

    tooltip = new TooltipPrivate(v, manual);
    tooltip->setView(v);
    tooltip->setTooltipText(text, kind);
    tooltip->place(pos);
    tooltip->show();
}
