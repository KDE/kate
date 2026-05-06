/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-FileCopyrightText: 2021 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: MIT
*/
#include "gitblametooltip.h"
#include "kategitblameplugin.h"

#include <QEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QScrollBar>
#include <QString>
#include <QStyle>
#include <QTextBrowser>
#include <QTimer>

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <KSyntaxHighlighting/AbstractHighlighter>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/State>

#include <KWindowSystem>

#include <ktexteditor_utils.h>

using KSyntaxHighlighting::AbstractHighlighter;
using KSyntaxHighlighting::Format;

static QString toHtmlRgbaString(const QColor &color)
{
    if (color.alpha() == 0xFF)
        return color.name();

    QString rgba = QStringLiteral("rgba(");
    rgba.append(QString::number(color.red()));
    rgba.append(u',');
    rgba.append(QString::number(color.green()));
    rgba.append(u',');
    rgba.append(QString::number(color.blue()));
    rgba.append(u',');
    // this must be alphaF
    rgba.append(QString::number(color.alphaF()));
    rgba.append(u')');
    return rgba;
}

class HtmlHl : public AbstractHighlighter
{
public:
    HtmlHl()
        : out(&outputString)
    {
    }

    void setText(const QString &txt)
    {
        text = txt;
        QTextStream in(&text);

        out.reset();
        outputString.clear();

        bool inDiff = false;

        KSyntaxHighlighting::State state;
        out << "<pre>";
        while (!in.atEnd()) {
            currentLine = in.readLine();

            // Link to open the tree view, insert as is
            if (currentLine.startsWith(QLatin1String("<a href"))) {
                out << currentLine;
                continue;
            }

            // allow empty lines in code blocks, no ruler here
            if (!inDiff && currentLine.isEmpty()) {
                out << "<hr>";
                continue;
            }

            // diff block
            if (!inDiff && currentLine.startsWith(QLatin1String("diff"))) {
                inDiff = true;
            }

            state = highlightLine(currentLine, state);
            out << "\n";
        }
        out << "</pre>";
    }

    QString html() const
    {
        //        while (!out.atEnd())
        //            qWarning() << out.readLine();
        return outputString;
    }

protected:
    void applyFormat(int offset, int length, const Format &format) override
    {
        if (!length) {
            return;
        }

        QString formatOutput;

        if (format.hasTextColor(theme())) {
            formatOutput = toHtmlRgbaString(format.textColor(theme()));
        }

        if (!formatOutput.isEmpty()) {
            out << "<span style=\"color:" << formatOutput << "\">";
        }

        out << currentLine.mid(offset, length).toHtmlEscaped();

        if (!formatOutput.isEmpty()) {
            out << "</span>";
        }
    }

private:
    QString text;
    QString currentLine;
    QString outputString;
    QTextStream out;
};

class GitBlameTooltipPrivate : public QTextBrowser
{
public:
    QKeySequence m_ignoreKeySequence;

    explicit GitBlameTooltipPrivate(KateGitBlamePluginView *pluginView)
        : QTextBrowser(nullptr)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::BypassGraphicsProxyWidget | Qt::ToolTip);
        setWordWrapMode(QTextOption::NoWrap);
        const auto margin = style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing, nullptr, this);
        document()->setDocumentMargin(margin);
        setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
        setProperty("_breeze_force_frame", true);
        setAttribute(Qt::WA_TranslucentBackground);
        setOpenLinks(false);
        connect(&m_hideTimer, &QTimer::timeout, this, &GitBlameTooltipPrivate::hideTooltip);

        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        m_htmlHl.setDefinition(m_syntaxHlRepo.definitionForName(QStringLiteral("Diff")));

        auto updateColors = [this](KTextEditor::Editor *e) {
            auto theme = e->theme();
            m_htmlHl.setTheme(theme);

            auto pal = palette();
            const QColor bg = theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor);
            pal.setColor(QPalette::Base, bg);
            const QColor normal = theme.textColor(KSyntaxHighlighting::Theme::Normal);
            pal.setColor(QPalette::Text, normal);
            setPalette(pal);

            setFont(Utils::editorFont());
        };
        updateColors(KTextEditor::Editor::instance());
        connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, updateColors);
        // Kinda ugly, but we are deep in the pimpl class wrapped by a normal cpp class...
        connect(this, &QTextBrowser::anchorClicked, pluginView, [pluginView, this](const QUrl &url) {
            hideTooltip();
            pluginView->showCommitTreeView(url);
        });
    }

    bool eventFilter(QObject *, QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::KeyPress:
        case QEvent::ShortcutOverride: {
            auto *ke = static_cast<QKeyEvent *>(event);
            if (ke->matches(QKeySequence::Copy)) {
                copy();
            } else if (ke->matches(QKeySequence::SelectAll)) {
                selectAll();
            }
            // hide the tooltip if it does not the focus
            if (!m_inFocus) {
                hideTooltip();
                return false;
            }
            event->accept();
            return true;
        }
        case QEvent::KeyRelease: {
            auto *ke = static_cast<QKeyEvent *>(event);
            if (ke->matches(QKeySequence::Copy) || ke->matches(QKeySequence::SelectAll)
                || (m_ignoreKeySequence.matches(QKeySequence(ke->key()) != QKeySequence::NoMatch)) || ke->key() == Qt::Key_Control || ke->key() == Qt::Key_Alt
                || ke->key() == Qt::Key_Shift || ke->key() == Qt::Key_AltGr || ke->key() == Qt::Key_Meta) {
                event->accept();
                return true;
            }
        } // fall through
        case QEvent::WindowActivate:
        case QEvent::WindowDeactivate:
            hideTooltip();
            break;
        case QEvent::MouseButtonPress:
            // hide the tooltip if it does not the focus
            if (!m_inFocus) {
                hideTooltip();
                return false;
            }
            break;
        default:
            break;
        }
        return false;
    }

    void showTooltip(const QString &text, KTextEditor::View *view)
    {
        if (text.isEmpty() || !view) {
            return;
        }

        m_htmlHl.setText(text);
        setHtml(m_htmlHl.html());
        // view changed?
        // => update definition
        // => update font
        if (view != m_view) {
            if (m_view && m_view->focusProxy()) {
                m_view->focusProxy()->removeEventFilter(this);
            }
            m_view = view;
            m_view->focusProxy()->installEventFilter(this);
        }

        // get a natural size for the document
        document()->adjustSize();
        QSize size = document()->size().toSize();

        const int contentsMarginsWidth = this->contentsMargins().left() + this->contentsMargins().right();
        const int contentsMarginsHeight = this->contentsMargins().top() + this->contentsMargins().bottom();
        const int docMargin = 2 * document()->documentMargin();
        int wMargins = contentsMarginsWidth + docMargin;
        int hMargins = contentsMarginsHeight;

        // add internal document padding and possible scrollbars to size
        size.setHeight(size.height() + hMargins);
        size.setWidth(size.width() + wMargins);

        int maxWidth = (m_view->window() ? m_view->window()->width() : m_view->width()) / 2;
        int maxHeight = (m_view->window() ? m_view->window()->height() : m_view->height()) / 2;

        // this makes it so the scrollbar margins are only added when we need a scrollbar
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        if ((size.height() - hMargins) > maxHeight) {
            setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            size.rwidth() += verticalScrollBar()->width();
            maxWidth += horizontalScrollBar()->width();
        }

        if ((size.width() - wMargins) > maxWidth) {
            setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            size.rheight() += horizontalScrollBar()->height();
            maxHeight += horizontalScrollBar()->height();
        }

        // limit the tool tip size; the resize call will respect these limits
        setMaximumSize(maxWidth, maxHeight);

        resize(size);

        auto cursor = m_view->cursorPosition();
        cursor.setColumn(m_view->document()->lineLength(cursor.line()));
        QPoint p = m_view->mapToGlobal(m_view->cursorToCoordinate(cursor));
        // FIXME: need to account for line height multiplier but ktexteditor doesn't expose that apparently :/
        auto fontConfig = m_view->configValue(QStringLiteral("font"));
        QFontMetrics fontMetris(fontConfig.value<QFont>());
        int lineHeight = fontMetris.height();

        p.setY(p.y() + lineHeight);
        p.setX(p.x() + fontMetris.horizontalAdvance(QChar::Space) * 4);

        auto screenSize = screen()->availableGeometry();
        // on wayland the tooltip can't overlap panels so it will get shifted and overlap the word we are hovering
        // so by clipping it to the window it wont overlap the panel (assuming the kate window is not overlapping the panel) and get shifted
        if (KWindowSystem::isPlatformWayland()) {
            screenSize = m_view->window()->geometry();
        }
        if (p.y() + this->height() > screenSize.y() + screenSize.height()) {
            p.ry() -= lineHeight + this->height();
        }

        this->move(p);

        show();
    }

    void hideTooltip()
    {
        if (m_view && m_view->focusProxy()) {
            m_view->focusProxy()->removeEventFilter(this);
            m_view.clear();
        }
        close();
        setText(QString());
        m_inContextMenu = false;
        m_inFocus = false;
    }

protected:
    void showEvent(QShowEvent *event) override
    {
        m_hideTimer.start(3000);
        QTextBrowser::showEvent(event);
    }

    void enterEvent(QEnterEvent *event) override
    {
        m_inContextMenu = false;
        m_inFocus = true;
        m_hideTimer.stop();
        QTextBrowser::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        m_inFocus = false;
        if (!m_hideTimer.isActive() && !m_inContextMenu && textCursor().selectionStart() == textCursor().selectionEnd()) {
            hideTooltip();
        }
        QTextBrowser::leaveEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        auto pos = event->pos();
        if (rect().contains(pos) || m_inContextMenu || textCursor().selectionStart() != textCursor().selectionEnd()) {
            QTextBrowser::mouseMoveEvent(event);
            return;
        }
        hideTooltip();
    }

    void contextMenuEvent(QContextMenuEvent *event) override
    {
        m_inContextMenu = true;
        QTextBrowser::contextMenuEvent(event);
    }

    void focusInEvent(QFocusEvent *) override
    {
        m_inFocus = true;
    }

    void focusOutEvent(QFocusEvent *) override
    {
        m_inFocus = false;
    }

private:
    bool m_inContextMenu = false;
    bool m_inFocus = false;
    QPointer<KTextEditor::View> m_view;
    QTimer m_hideTimer;
    HtmlHl m_htmlHl;
    KSyntaxHighlighting::Repository m_syntaxHlRepo;
};

GitBlameTooltip::GitBlameTooltip(KateGitBlamePluginView *pv)
    : m_pluginView(pv)

{
}

GitBlameTooltip::~GitBlameTooltip() = default;

void GitBlameTooltip::show(const QString &text, KTextEditor::View *view)
{
    if (text.isEmpty() || !view || !view->document()) {
        return;
    }

    if (!d) {
        d = std::make_unique<GitBlameTooltipPrivate>(m_pluginView);
    }

    d->showTooltip(text, view);
}

void GitBlameTooltip::hide()
{
    if (!d) {
        return;
    }
    d->hideTooltip();
}

void GitBlameTooltip::setIgnoreKeySequence(const QKeySequence &sequence)
{
    if (!d) {
        d = std::make_unique<GitBlameTooltipPrivate>(m_pluginView);
    }
    d->m_ignoreKeySequence = sequence;
}
