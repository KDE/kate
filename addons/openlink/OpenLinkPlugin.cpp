/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "OpenLinkPlugin.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KTextEditor/Document>
#include <KTextEditor/TextHintInterface>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

#include <QDesktopServices>
#include <QEvent>
#include <QFileInfo>
#include <QMouseEvent>
#include <QToolTip>
#include <QUrl>

class OpenLinkTextHint : public KTextEditor::TextHintProvider
{
    OpenLinkPluginView *m_pview;
    QPointer<KTextEditor::View> m_view;

public:
    OpenLinkTextHint(OpenLinkPluginView *pview)
        : m_pview(pview)
    {
    }

    ~OpenLinkTextHint() override
    {
        if (m_view) {
            m_view->unregisterTextHintProvider(this);
        }
    }

    void setView(KTextEditor::View *v)
    {
        if (m_view) {
            m_view->unregisterTextHintProvider(this);
        }
        if (v) {
            m_view = v;
            m_view->registerTextHintProvider(this);
        }
    }

    QString textHint(KTextEditor::View *view, const KTextEditor::Cursor &position) override
    {
        auto doc = view->document();
        // If the cursor is inside the link area, show a hint to the user
        auto currentPos = view->cursorPosition();
        auto it = m_pview->m_docHighligtedLinkRanges.find(doc);
        if (it != m_pview->m_docHighligtedLinkRanges.end()) {
            const auto &ranges = it->second;
            for (const auto &range : ranges) {
                if (range && range->contains(position) && range->contains(currentPos)) {
                    const QString hint = QStringLiteral("<p>") + i18n("Ctrl+Click to open link") + QStringLiteral("</p>");
                    return hint;
                }
            }
        }
        return {};
    }
};

// TODO: Support file urls
// TODO: Allow the user to specify url matching re
// so that we can click on stuff like BUG-123123 and go to bugs.blah.org/123123
// re windows [a-zA-Z]:[\\\/](?:[a-zA-Z0-9]+[\\\/])*([a-zA-Z0-9]+)

K_PLUGIN_FACTORY_WITH_JSON(OpenLinkPluginFactory, "OpenLinkPlugin.json", registerPlugin<OpenLinkPlugin>();)

class GotoLinkHover : public QObject
{
public:
    void highlight(KTextEditor::View *activeView, KTextEditor::Range range)
    {
        if (!activeView || !activeView->document() || !viewInternal) {
            return;
        }

        viewInternal->setCursor(Qt::PointingHandCursor);
        // underline the hovered word
        auto doc = activeView->document();
        if (!m_movingRange || doc != m_movingRange->document()) {
            m_movingRange.reset(doc->newMovingRange(range));
            // clang-format off
            connect(doc, &KTextEditor::Document::aboutToInvalidateMovingInterfaceContent, this, &GotoLinkHover::clearMovingRange, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::aboutToDeleteMovingInterfaceContent, this, &GotoLinkHover::clearMovingRange, Qt::UniqueConnection);
            // clang-format on
        } else {
            m_movingRange->setRange(range);
        }

        static const KTextEditor::Attribute::Ptr attr([] {
            auto attr = new KTextEditor::Attribute;
            // Bluish, works with light/dark bg
            attr->setForeground(QColor(0x409DFF));
            return attr;
        }());
        m_movingRange->setAttribute(attr);
    }

    void clear()
    {
        if (m_movingRange) {
            m_movingRange->setRange(KTextEditor::Range::invalid());
        }
        if (viewInternal && viewInternal->cursor() != Qt::IBeamCursor) {
            viewInternal->setCursor(Qt::IBeamCursor);
        }
        viewInternal.clear();
        currentWord.clear();
    }

    QString currentWord;
    QPointer<QWidget> viewInternal;

private:
    void clearMovingRange(KTextEditor::Document *doc)
    {
        if (m_movingRange && m_movingRange->document() == doc) {
            m_movingRange.reset();
        }
    }

private:
    std::unique_ptr<KTextEditor::MovingRange> m_movingRange;
};

QObject *OpenLinkPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new OpenLinkPluginView(this, mainWindow);
}

OpenLinkPluginView::OpenLinkPluginView(OpenLinkPlugin *plugin, KTextEditor::MainWindow *mainWin)
    : QObject(plugin)
    , m_mainWindow(mainWin)
    , m_ctrlHoverFeedback(new GotoLinkHover())
    , m_textHintProvider(new OpenLinkTextHint(this))
{
    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &OpenLinkPluginView::onActiveViewChanged);
    onActiveViewChanged(m_mainWindow->activeView());
    m_mainWindow->guiFactory()->addClient(this);
}

OpenLinkPluginView::~OpenLinkPluginView()
{
    m_textHintProvider->setView(nullptr);
    delete m_textHintProvider;
    disconnect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &OpenLinkPluginView::onActiveViewChanged);
    onActiveViewChanged(nullptr);
    m_mainWindow->guiFactory()->removeClient(this);
}

void OpenLinkPluginView::onActiveViewChanged(KTextEditor::View *view)
{
    auto oldView = m_activeView;
    if (oldView == view) {
        return;
    }
    m_activeView = view;
    m_textHintProvider->setView(view);

    if (view && view->focusProxy()) {
        view->focusProxy()->installEventFilter(this);
        connect(view, &KTextEditor::View::verticalScrollPositionChanged, this, &OpenLinkPluginView::onViewScrolled);
        onViewScrolled();

        auto doc = view->document();
        connect(doc, &KTextEditor::Document::textInserted, this, &OpenLinkPluginView::onTextInserted);
        connect(doc, &KTextEditor::Document::textRemoved, this, &OpenLinkPluginView::onTextRemoved);

        connect(doc, &KTextEditor::Document::aboutToInvalidateMovingInterfaceContent, this, &OpenLinkPluginView::clear, Qt::UniqueConnection);
        connect(doc, &KTextEditor::Document::aboutToDeleteMovingInterfaceContent, this, &OpenLinkPluginView::clear, Qt::UniqueConnection);
    }

    if (oldView && oldView->focusProxy()) {
        oldView->focusProxy()->removeEventFilter(this);
        disconnect(oldView, &KTextEditor::View::verticalScrollPositionChanged, this, &OpenLinkPluginView::onViewScrolled);
        disconnect(oldView->document(), &KTextEditor::Document::textInserted, this, &OpenLinkPluginView::onTextInserted);
        disconnect(oldView->document(), &KTextEditor::Document::textRemoved, this, &OpenLinkPluginView::onTextRemoved);
    }
}

void OpenLinkPluginView::clear(KTextEditor::Document *doc)
{
    m_docHighligtedLinkRanges.erase(doc);
}

void OpenLinkPluginView::gotoLink()
{
    const QUrl u = QUrl::fromUserInput(m_ctrlHoverFeedback->currentWord);
    if (u.isValid()) {
        QDesktopServices::openUrl(u);
    }
}

static const QRegularExpression &linkRE()
{
    static const QRegularExpression re(
        QStringLiteral(R"re((https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*)))re"));
    return re;
}

static void adjustMDLink(const QString &line, int capturedStart, int &capturedEnd)
{
    if (capturedStart > 1) { // at least two chars before
        int i = capturedStart - 1;
        // for markdown [asd](google.com) style urls, make sure to strip last `)`
        bool isMD = line.at(i - 1) == QLatin1Char(']') && line.at(i) == QLatin1Char('(');
        if (isMD) {
            int f = line.lastIndexOf(QLatin1Char(')'), capturedEnd >= line.size() ? line.size() - 1 : capturedEnd);
            capturedEnd = f != -1 ? f : capturedEnd;
        }
    }
}

void OpenLinkPluginView::highlightIfLink(KTextEditor::Cursor c, QWidget *viewInternal)
{
    if (!m_activeView || !m_activeView->document() || !c.isValid()) {
        return;
    }

    const auto doc = m_activeView->document();
    const QString line = doc->line(c.line());
    if (c.column() >= line.size()) {
        return;
    }

    auto match = linkRE().match(line);
    const int capturedStart = match.capturedStart();
    int capturedEnd = match.capturedEnd();

    if (match.hasMatch() && capturedStart <= c.column() && c.column() <= capturedEnd) {
        adjustMDLink(line, capturedStart, capturedEnd);
        m_ctrlHoverFeedback->currentWord = line.mid(capturedStart, capturedEnd - capturedStart);
        m_ctrlHoverFeedback->viewInternal = viewInternal;
        KTextEditor::Range range(c.line(), capturedStart, c.line(), capturedEnd);
        m_ctrlHoverFeedback->highlight(m_activeView, range);
    }
}

void OpenLinkPluginView::onTextInserted(KTextEditor::Document *doc, KTextEditor::Cursor pos, const QString &text)
{
    if (doc == m_activeView->document()) {
        KTextEditor::Range range(pos, pos);
        int newlines = text.count(QLatin1Char('\n'));
        pos.setLine(pos.line() + newlines);
        range.setEnd(pos);
        highlightLinks(range);
    }
}

void OpenLinkPluginView::onTextRemoved(KTextEditor::Document *doc, KTextEditor::Range range, const QString &)
{
    if (doc == m_activeView->document()) {
        highlightLinks(range);
    }
}

void OpenLinkPluginView::onViewScrolled()
{
    highlightLinks(KTextEditor::Range::invalid());
}

void OpenLinkPluginView::highlightLinks(KTextEditor::Range range)
{
    if (!m_activeView) {
        return;
    }
    const auto lineRange = range.toLineRange();

    const int startLine = lineRange.isValid() ? lineRange.start() : m_activeView->firstDisplayedLine();
    const int endLine = lineRange.isValid() ? lineRange.end() : m_activeView->lastDisplayedLine();
    auto doc = m_activeView->document();
    auto &ranges = m_docHighligtedLinkRanges[doc];
    if (lineRange.isValid()) {
        ranges.erase(std::remove_if(ranges.begin(),
                                    ranges.end(),
                                    [lineRange](const std::unique_ptr<KTextEditor::MovingRange> &r) {
                                        return lineRange.overlapsLine(r->start().line());
                                    }),
                     ranges.end());
    } else {
        ranges.clear();
    }
    // Loop over visible lines and highlight links
    for (int i = startLine; i <= endLine; ++i) {
        const QString line = doc->line(i);
        QRegularExpressionMatchIterator it = linkRE().globalMatch(line);
        while (it.hasNext()) {
            auto match = it.next();
            if (match.hasMatch()) {
                int capturedEnd = match.capturedEnd();
                adjustMDLink(line, match.capturedStart(), capturedEnd);
                KTextEditor::Range range(i, match.capturedStart(), i, capturedEnd);
                KTextEditor::MovingRange *r = doc->newMovingRange(range);
                static const KTextEditor::Attribute::Ptr attr([] {
                    auto attr = new KTextEditor::Attribute;
                    attr->setUnderlineStyle(QTextCharFormat::SingleUnderline);
                    return attr;
                }());
                r->setAttribute(attr);
                ranges.emplace_back(r);
                // qDebug() << match.captured() << match.capturedStart() << match.capturedEnd();
            }
        }
    }
}

bool OpenLinkPluginView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Leave) {
        m_ctrlHoverFeedback->clear();
        return QObject::eventFilter(obj, event);
    }

    if (event->type() != QEvent::MouseMove && event->type() != QEvent::MouseButtonRelease) {
        return QObject::eventFilter(obj, event);
    }

    auto mouseEvent = static_cast<QMouseEvent *>(event);

    // The user pressed Ctrl + Click
    if (event->type() == QEvent::MouseButtonRelease && !m_ctrlHoverFeedback->currentWord.isEmpty()) {
        if (mouseEvent->button() == Qt::LeftButton && mouseEvent->modifiers() == Qt::ControlModifier) {
            gotoLink();
            m_ctrlHoverFeedback->clear();
            return true;
        }
    }
    // The user is hovering with Ctrl pressed
    else if (event->type() == QEvent::MouseMove) {
        if (mouseEvent->modifiers() == Qt::ControlModifier) {
            auto viewInternal = static_cast<QWidget *>(obj);
            const QPoint coords = viewInternal->mapTo(m_activeView, mouseEvent->pos());
            const KTextEditor::Cursor cur = m_activeView->coordinatesToCursor(coords);
            if (!cur.isValid() || m_activeView->document()->wordRangeAt(cur).isEmpty()) {
                return false;
            }
            highlightIfLink(cur, viewInternal);
        } else {
            // if there is no word or simple mouse move, make sure to unset the cursor and remove the highlight
            if (m_ctrlHoverFeedback->viewInternal) {
                m_ctrlHoverFeedback->clear();
            }
        }
    }
    return false;
}

#include "OpenLinkPlugin.moc"
#include "moc_OpenLinkPlugin.cpp"
