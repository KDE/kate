/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "OpenLinkPlugin.h"
#include "matchers.h"

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
#if KTEXTEDITOR_VERSION < QT_VERSION_CHECK(6, 9, 0)
            connect(doc, &KTextEditor::Document::aboutToDeleteMovingInterfaceContent, this, &GotoLinkHover::clearMovingRange, Qt::UniqueConnection);
#endif
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
        link.clear();
    }

    OpenLinkType linkType = HttpLink;
    QString link;
    KTextEditor::Cursor startPos = KTextEditor::Cursor::invalid();
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
#if KTEXTEDITOR_VERSION < QT_VERSION_CHECK(6, 9, 0)
        connect(doc, &KTextEditor::Document::aboutToDeleteMovingInterfaceContent, this, &OpenLinkPluginView::clear, Qt::UniqueConnection);
#endif
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
    const QUrl u = QUrl::fromUserInput(m_ctrlHoverFeedback->link);
    if (m_ctrlHoverFeedback->linkType == HttpLink) {
        if (u.isValid()) {
            QDesktopServices::openUrl(u);
        }
    } else if (m_ctrlHoverFeedback->linkType == FileLink) {
        if (auto v = m_mainWindow->openUrl(u)) {
            if (m_ctrlHoverFeedback->startPos.isValid()) {
                auto pos = m_ctrlHoverFeedback->startPos;
                pos.setLine(pos.line() - 1);
                if (pos.column() > 0) {
                    pos.setColumn(pos.column() - 1);
                }
                v->setCursorPosition(pos);
            }
        }
    } else {
        Q_ASSERT(false);
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

    std::vector<OpenLinkRange> matchedRanges;
    matchLine(line, &matchedRanges);
    for (auto [start, end, link, startPos, type] : matchedRanges) {
        if (start <= c.column() && c.column() <= end) {
            m_ctrlHoverFeedback->link = link;
            m_ctrlHoverFeedback->viewInternal = viewInternal;
            m_ctrlHoverFeedback->linkType = type;
            m_ctrlHoverFeedback->startPos = startPos;
            KTextEditor::Range range(c.line(), start, c.line(), end);
            m_ctrlHoverFeedback->highlight(m_activeView, range);
            break;
        }
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

static KTextEditor::MovingRange *highlightRange(KTextEditor::Document *doc, KTextEditor::Range range)
{
    KTextEditor::MovingRange *r = doc->newMovingRange(range);
    static const KTextEditor::Attribute::Ptr attr([] {
        auto attr = new KTextEditor::Attribute;
        attr->setUnderlineStyle(QTextCharFormat::SingleUnderline);
        return attr;
    }());
    r->setAttribute(attr);
    return r;
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
        std::erase_if(ranges, [lineRange](const std::unique_ptr<KTextEditor::MovingRange> &r) {
            return lineRange.overlapsLine(r->start().line());
        });
    } else {
        ranges.clear();
    }
    // Loop over visible lines and highlight links
    int linesChecked = 0;
    std::vector<OpenLinkRange> matchedRanges;
    for (int i = startLine; i <= endLine; ++i, ++linesChecked) {
        // avoid checking too many lines
        if (linesChecked > 400) {
            break;
        }
        const QString line = doc->line(i);
        matchLine(line, &matchedRanges);
        for (auto [startCol, endCol, link, startCursor, _] : matchedRanges) {
            Q_UNUSED(startCursor)
            Q_UNUSED(link)
            KTextEditor::Range range(i, startCol, i, endCol);
            ranges.emplace_back(highlightRange(doc, range));
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
    if (event->type() == QEvent::MouseButtonRelease && !m_ctrlHoverFeedback->link.isEmpty()) {
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
