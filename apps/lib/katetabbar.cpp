/*
    SPDX-FileCopyrightText: 2014 Dominik Haumann <dhaumann@kde.org>
    SPDX-FileCopyrightText: 2020 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katetabbar.h"
#include "kateapp.h"
#include "ktexteditor_utils.h"
#include "tabmimedata.h"

#include <QApplication>
#include <QDataStream>
#include <QDrag>
#include <QPixmap>
#include <QStyleOptionTab>
#include <QStylePainter>
#include <QWheelEvent>

#include <KAcceleratorManager>
#include <KConfigGroup>
#include <KSharedConfig>

#include <KTextEditor/Document>

/**
 * Creates a new tab bar with the given \a parent.
 */
KateTabBar::KateTabBar(QWidget *parent)
    : QTabBar(parent)
{
    // we want no auto-accelerators here
    KAcceleratorManager::setNoAccel(this);

    // enable document mode, docs tell this will trigger:
    // On macOS this will look similar to the tabs in Safari or Sierra's Terminal.app.
    // this seems reasonable for our document tabs
    setDocumentMode(true);

    // we want drag and drop
    setAcceptDrops(true);

    // allow users to re-arrange the tabs
    setMovable(true);

    // enforce configured limit
    readConfig();

    // handle config changes
    connect(KateApp::self(), &KateApp::configurationChanged, this, &KateTabBar::readConfig);
}

void KateTabBar::readConfig()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, "General");

    // 0 == unlimited, normalized other inputs
    const int tabCountLimit = std::max(cgGeneral.readEntry("Tabbar Tab Limit", 0), 0);
    if (m_tabCountLimit != tabCountLimit) {
        m_tabCountLimit = tabCountLimit;
        const QVector<DocOrWidget> docList = documentList();
        if (m_tabCountLimit > 0 && docList.count() > m_tabCountLimit) {
            // close N least used tabs
            QMap<quint64, DocOrWidget> lruDocs;
            for (const DocOrWidget &doc : docList) {
                lruDocs[m_docToLruCounterAndHasTab[doc].first] = doc;
            }
            int toRemove = docList.count() - m_tabCountLimit;
            for (DocOrWidget doc : std::as_const(lruDocs)) {
                if (toRemove-- == 0) {
                    break;
                }
                removeTab(documentIdx(doc));
            }
        } else if (m_docToLruCounterAndHasTab.size() > (size_t)docList.size()) {
            // populate N recently user documents
            std::map<quint64, DocOrWidget, std::greater<quint64>> mruDocs;
            for (const auto &i : m_docToLruCounterAndHasTab) {
                DocOrWidget doc = i.first;
                if (!docList.contains(doc)) {
                    mruDocs[i.second.first] = doc;
                }
            }
            int toAdd = m_tabCountLimit - docList.count();
            for (const auto &i : mruDocs) {
                if (toAdd-- == 0) {
                    break;
                }
                DocOrWidget doc = i.second;
                const auto idx = addTab(doc.doc() ? doc.doc()->documentName() : doc.widget()->windowTitle());
                setTabDocument(idx, doc);
            }
        }
    }

    // use scroll buttons if we have no limit
    setUsesScrollButtons(m_tabCountLimit == 0 || cgGeneral.readEntry("Allow Tab Scrolling", true));

    // elide if requested, this is independent of the limit, just honor the users wish
    setElideMode(cgGeneral.readEntry("Elide Tab Text", false) ? Qt::ElideMiddle : Qt::ElideNone);

    // handle tab close button and expansion
    setExpanding(cgGeneral.readEntry("Expand Tabs", false));
    setTabsClosable(cgGeneral.readEntry("Show Tabs Close Button", true));

    // get mouse click rules
    m_doubleClickNewDocument = cgGeneral.readEntry("Tab Double Click New Document", true);
    m_middleClickCloseDocument = cgGeneral.readEntry("Tab Middle Click Close Document", true);
}

void KateTabBar::setActive(bool active)
{
    if (active == m_isActive) {
        return;
    }
    m_isActive = active;
    update();
}

bool KateTabBar::isActive() const
{
    return m_isActive;
}

int KateTabBar::prevTab() const
{
    return currentIndex() == 0 ? 0 // first index, keep it here.
                               : currentIndex() - 1;
}

int KateTabBar::nextTab() const
{
    return currentIndex() == count() - 1 ? count() - 1 // last index, keep it here.
                                         : currentIndex() + 1;
}

bool KateTabBar::containsTab(int index) const
{
    return index >= 0 && index < count();
}

QVariant KateTabBar::ensureValidTabData(int idx)
{
    if (!tabData(idx).isValid()) {
        DocOrWidget v(static_cast<KTextEditor::Document *>(nullptr));
        setTabData(idx, QVariant::fromValue(v));
    }
    return tabData(idx);
}

void KateTabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->accept();

    if (m_doubleClickNewDocument && event->button() == Qt::LeftButton) {
        Q_EMIT newTabRequested();
    }
}

void KateTabBar::mousePressEvent(QMouseEvent *event)
{
    if (!isActive()) {
        Q_EMIT activateViewSpaceRequested();
    }

    int tab = tabAt(event->pos());
    if (event->button() == Qt::LeftButton && tab != -1 && count() > 1) {
        dragStartPos = event->pos();
        auto r = tabRect(tab);
        dragHotspotPos = {dragStartPos.x() - r.x(), dragStartPos.y() - r.y()};
    } else {
        dragStartPos = {};
    }

    QTabBar::mousePressEvent(event);

    // handle close for middle mouse button
    if (m_middleClickCloseDocument && event->button() == Qt::MiddleButton) {
        int id = tabAt(event->pos());
        if (id >= 0) {
            Q_EMIT tabCloseRequested(id);
        }
    }
}

void KateTabBar::mouseMoveEvent(QMouseEvent *event)
{
    if (dragStartPos.isNull()) {
        QTabBar::mouseMoveEvent(event);
        return;
    }

    // We start our drag once the cursor leaves "current view space"
    // Starting drag before that is a bit useless
    // One disadvantage of the current approach is that the user
    // might not know that kate's tabs can be dragged to other
    // places, unless they drag it really far away
    auto viewSpace = qobject_cast<KateViewSpace *>(parentWidget());
    const auto viewspaceRect = viewSpace->rect();
    QRect viewspaceRectTopArea = viewspaceRect;
    viewspaceRectTopArea.setTop(viewspaceRect.top() - 40);
    viewspaceRectTopArea.setBottom(viewspaceRect.height() / 2);
    if (!viewSpace || viewspaceRectTopArea.contains(event->pos())) {
        QTabBar::mouseMoveEvent(event);
        return;
    }

    if ((event->pos() - dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
        QTabBar::mouseMoveEvent(event);
        return;
    }

    if (rect().contains(event->pos())) {
        return QTabBar::mouseMoveEvent(event);
    }

    int tab = currentIndex();
    if (tab < 0) {
        return;
    }

    QRect rect = tabRect(tab);
    const auto tabData = this->tabData(tab).value<DocOrWidget>();
    auto *tabObject = tabData.qobject();
    if (!tabObject) {
        return;
    }

    QPixmap p(rect.size() * this->devicePixelRatioF());
    p.setDevicePixelRatio(this->devicePixelRatioF());
    p.fill(Qt::transparent);

    // For some reason initStyleOption with tabIdx directly
    // wasn't working, so manually set some stuff
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QStyleOptionTabV4 opt;
#else
    QStyleOptionTab opt;
#endif
    opt.text = tabText(tab);
    opt.icon = tabIcon(tab);
    opt.iconSize = iconSize();
    opt.state = QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_Raised;
    opt.tabIndex = tab;
    opt.position = QStyleOptionTab::OnlyOneTab;
    opt.features = QStyleOptionTab::TabFeature::HasFrame;
    opt.rect = rect;
    // adjust the rect so that it starts at(0,0)
    opt.rect.adjust(-opt.rect.x(), 0, -opt.rect.x(), 0);

    QStylePainter paint(&p, this);
    paint.drawControl(QStyle::CE_TabBarTab, opt);
    paint.end();

    QByteArray data;
    auto view = viewSpace->currentView();
    if (view) {
        KTextEditor::Cursor cp = view->cursorPosition();
        QDataStream ds(&data, QIODevice::WriteOnly);
        ds << cp.line();
        ds << cp.column();
        ds << view->document()->url();
    } else if (!viewSpace->currentWidget()) {
        qWarning() << "No view or widget, why?";
        return;
    }

    auto mime = new TabMimeData(viewSpace, tabData);
    mime->setData(QStringLiteral("application/kate.tab.mimedata"), data);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mime);
    drag->setPixmap(p);
    drag->setHotSpot(dragHotspotPos);

    auto posCopy = dragStartPos;
    dragStartPos = {};
    dragHotspotPos = {};
    drag->exec(Qt::CopyAction);

    // On drag end we check whether the drag was a success
    // i.e., if the widget/doc in tabData still exists in
    // this tabbar, it failed or there was a copy operation
    // in which case there might be the "movable tab" hanging
    // in between which we reset
    auto onDragEnd = [this, tabObject, posCopy]() {
        bool found = false;
        int tabIdx = 0;
        for (; tabIdx < count(); ++tabIdx) {
            auto d = this->tabData(tabIdx);
            // We only expect doc, no dnd support for widgets
            if (d.value<DocOrWidget>().qobject() == tabObject) {
                found = true;
                break;
            }
        }

        if (found) {
            // We send this even to ensure the "moveable tab" is properly reset and we have no dislocated tabs
            auto e = QMouseEvent(QEvent::MouseButtonPress, posCopy, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            qApp->sendEvent(this, &e);
            if (currentIndex() != tabIdx) {
                setCurrentIndex(tabIdx);
            }
        }
    };

    connect(drag, &QDrag::destroyed, this, onDragEnd);
}

void KateTabBar::contextMenuEvent(QContextMenuEvent *ev)
{
    int id = tabAt(ev->pos());
    if (id >= 0) {
        Q_EMIT contextMenuRequest(id, ev->globalPos());
    }
}

void KateTabBar::setTabDocument(int idx, DocOrWidget d)
{
    setTabData(idx, QVariant::fromValue(d));

    auto *doc = d.doc();
    // BUG: 441340 We need to escape the & because it is used for accelerators/shortcut mnemonic by default
    QString tabName = d.doc() ? d.doc()->documentName() : d.widget()->windowTitle();
    tabName.replace(QLatin1Char('&'), QLatin1String("&&"));
    setTabText(idx, tabName);
    if (d.doc()) {
        setTabToolTip(idx, doc->url().isValid() ? doc->url().toDisplayString(QUrl::PreferLocalFile) : doc->documentName());
        setModifiedStateIcon(idx, doc);
    } else {
        setTabIcon(idx, d.widget()->windowIcon());
    }
}

void KateTabBar::setModifiedStateIcon(int idx, KTextEditor::Document *doc)
{
    // use common method, as e.g. in filetree plugin, too
    setTabIcon(idx, Utils::iconForDocument(doc));
}

void KateTabBar::setCurrentDocument(DocOrWidget docOrWidget)
{
    // in any case: update lru counter for this document, might add new element to hash
    // we have a tab after this call, too!
    m_docToLruCounterAndHasTab[docOrWidget] = std::make_pair(++m_lruCounter, true);

    // do we have a tab for this document?
    // if yes => just set as current one
    const int existingIndex = documentIdx(docOrWidget);
    if (existingIndex != -1) {
        setCurrentIndex(existingIndex);
        return;
    }

    // else: if we are still inside the allowed number of tabs or have no limit
    // => create new tab and be done
    if ((m_tabCountLimit == 0) || (count() < m_tabCountLimit)) {
        m_beingAdded = docOrWidget;
        insertTab(-1, docOrWidget.doc() ? docOrWidget.doc()->documentName() : docOrWidget.widget()->windowTitle());
        return;
    }

    // ok, we have already the limit of tabs reached:
    // replace the tab with the lowest lru counter => the least recently used

    // search for the right tab
    quint64 minCounter = static_cast<quint64>(-1);
    int indexToReplace = 0;
    DocOrWidget docToReplace = DocOrWidget::null();
    for (int idx = 0; idx < count(); idx++) {
        QVariant data = tabData(idx);
        DocOrWidget doc = data.value<DocOrWidget>();
        const quint64 currentCounter = m_docToLruCounterAndHasTab[doc].first;
        if (currentCounter <= minCounter) {
            minCounter = currentCounter;
            indexToReplace = idx;
            docToReplace = doc;
        }
    }

    // mark the replace doc as "has no tab"
    m_docToLruCounterAndHasTab[docToReplace].second = false;

    // replace it's data + set it as active
    setTabDocument(indexToReplace, docOrWidget);
    setCurrentIndex(indexToReplace);
}

void KateTabBar::removeDocument(DocOrWidget doc)
{
    // purge LRU storage, must work
    auto erased = (m_docToLruCounterAndHasTab.erase(doc) == 1);
    if (!erased) {
        qWarning() << Q_FUNC_INFO << "Failed to erase";
    }

    // remove document if needed, we might have no tab for it, if tab count is limited!
    const int idx = documentIdx(doc);
    if (idx == -1) {
        return;
    }

    // if we have some tab limit, replace the removed tab with the next best document that has none!
    if (m_tabCountLimit > 0) {
        quint64 maxCounter = 0;
        DocOrWidget docToReplace = DocOrWidget::null();
        for (const auto &lru : m_docToLruCounterAndHasTab) {
            // ignore stuff with tabs
            if (lru.second.second) {
                continue;
            }

            // search most recently used one
            if (lru.second.first >= maxCounter) {
                maxCounter = lru.second.first;
                docToReplace = lru.first;
            }
        }

        // any document found? replace the tab we want to close and be done
        if (!docToReplace.isNull()) {
            // mark the replace doc as "has a tab"
            m_docToLruCounterAndHasTab[docToReplace].second = true;

            // replace info for the tab
            setTabDocument(idx, docToReplace);
            setCurrentIndex(idx);
            Q_EMIT currentChanged(idx);
            return;
        }
    }

    // if we arrive here, we just need to purge the tab
    // this happens if we have no limit or no document to replace the current one
    removeTab(idx);
}

int KateTabBar::documentIdx(DocOrWidget doc)
{
    for (int idx = 0; idx < count(); idx++) {
        const QVariant data = tabData(idx);
        if (data.value<DocOrWidget>().qobject() == doc.qobject()) {
            return idx;
        }
    }
    return -1;
}

DocOrWidget KateTabBar::tabDocument(int idx)
{
    QVariant data = ensureValidTabData(idx);
    DocOrWidget buttonData = data.value<DocOrWidget>();

    auto doc = DocOrWidget::null();
    // The tab got activated before the correct finalization,
    // we need to plug the document before returning.
    if (buttonData.isNull() && !m_beingAdded.isNull()) {
        setTabDocument(idx, m_beingAdded);
        doc = m_beingAdded;
        m_beingAdded.clear();
    } else {
        doc = buttonData;
    }

    return doc;
}

void KateTabBar::tabInserted(int idx)
{
    if (m_beingAdded.qobject()) {
        setTabDocument(idx, m_beingAdded);
    }
    m_beingAdded.clear();
}

QVector<DocOrWidget> KateTabBar::documentList() const
{
    QVector<DocOrWidget> result;
    result.reserve(count());
    for (int idx = 0; idx < count(); idx++) {
        QVariant data = tabData(idx);
        result.append(data.value<DocOrWidget>());
    }
    return result;
}
