/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "OpenLinkPlugin.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

#include <QDesktopServices>
#include <QEvent>
#include <QFileInfo>
#include <QMouseEvent>
#include <QUrl>

// TODO: Support file urls
// TODO: Allow the user to specify url matching re
// so that we can click on stuff like BUG-123123 and go to bugs.blah.org/123123
// re windows [a-zA-Z]:[\\\/](?:[a-zA-Z0-9]+[\\\/])*([a-zA-Z0-9]+)

K_PLUGIN_FACTORY_WITH_JSON(OpenLinkPluginFactory, "OpenLinkPlugin.json", registerPlugin<OpenLinkPlugin>();)

class GotoLinkHover : public QObject
{
    Q_OBJECT
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
            m_movingRange.reset(qobject_cast<KTextEditor::MovingInterface *>(doc)->newMovingRange(range));
            // clang-format off
            connect(doc,
                    SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document*)),
                    this,
                    SLOT(clear(KTextEditor::Document*)),
                    Qt::UniqueConnection);
            connect(doc,
                    SIGNAL(aboutToDeleteMovingInterfaceContent(KTextEditor::Document*)),
                    this,
                    SLOT(clear(KTextEditor::Document*)),
                    Qt::UniqueConnection);
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
    Q_SLOT void clear(KTextEditor::Document *doc)
    {
        if (m_movingRange->document() == doc) {
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
{
    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &OpenLinkPluginView::onActiveViewChanged);
    onActiveViewChanged(m_mainWindow->activeView());
    m_mainWindow->guiFactory()->addClient(this);
}

OpenLinkPluginView::~OpenLinkPluginView()
{
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

    if (view && view->focusProxy()) {
        view->focusProxy()->installEventFilter(this);
        connect(view, &KTextEditor::View::verticalScrollPositionChanged, this, &OpenLinkPluginView::onViewScrolled);
        connect(view, &KTextEditor::View::textInserted, this, &OpenLinkPluginView::onTextInserted);
        onViewScrolled();
        auto doc = view->document();
        connect(doc,
                SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document *)),
                this,
                SLOT(clear(KTextEditor::Document *)),
                Qt::UniqueConnection);
        connect(doc, SIGNAL(aboutToDeleteMovingInterfaceContent(KTextEditor::Document *)), this, SLOT(clear(KTextEditor::Document *)), Qt::UniqueConnection);
    }

    if (oldView && oldView->focusProxy()) {
        oldView->focusProxy()->removeEventFilter(this);
        disconnect(view, &KTextEditor::View::verticalScrollPositionChanged, this, &OpenLinkPluginView::onViewScrolled);
        disconnect(view, &KTextEditor::View::textInserted, this, &OpenLinkPluginView::onTextInserted);
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

    int spaceBefore = line.lastIndexOf(QLatin1Char(' '), c.column());
    const int spaceAfter = line.indexOf(QLatin1Char(' '), c.column());
    if (spaceBefore == spaceAfter)
        return;
    spaceBefore++; // Skip the space

    const QString word = line.mid(spaceBefore, spaceAfter == -1 ? spaceAfter : spaceAfter - spaceBefore);
    if (word.isEmpty()) {
        return;
    }
    const int sc = spaceBefore;
    KTextEditor::Range range(c.line(), sc, c.line(), spaceAfter == -1 ? line.size() : spaceAfter);

    if (linkRE().match(word).hasMatch()) {
        m_ctrlHoverFeedback->currentWord = word;
        m_ctrlHoverFeedback->viewInternal = viewInternal;
        m_ctrlHoverFeedback->highlight(m_activeView, range);
    }
}

void OpenLinkPluginView::onTextInserted(KTextEditor::View *view, const KTextEditor::Cursor &position, const QString &)
{
    if (view == m_activeView) {
        highlightLinks(position);
    }
}

void OpenLinkPluginView::onViewScrolled()
{
    highlightLinks(KTextEditor::Cursor::invalid());
}

void OpenLinkPluginView::highlightLinks(KTextEditor::Cursor pos)
{
    if (!m_activeView) {
        return;
    }

    const int startLine = pos.isValid() ? pos.line() : m_activeView->firstDisplayedLine();
    const int endLine = pos.isValid() ? pos.line() : m_activeView->lastDisplayedLine();
    auto doc = m_activeView->document();
    auto &ranges = m_docHighligtedLinkRanges[doc];
    auto iface = qobject_cast<KTextEditor::MovingInterface *>(doc);
    if (pos.isValid()) {
        ranges.erase(std::remove_if(ranges.begin(),
                                    ranges.end(),
                                    [line = pos.line()](const std::unique_ptr<KTextEditor::MovingRange> &r) {
                                        return r && r->start().line() == line;
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
                KTextEditor::Range range(i, match.capturedStart(), i, match.capturedEnd());
                KTextEditor::MovingRange *r = iface->newMovingRange(range);
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
