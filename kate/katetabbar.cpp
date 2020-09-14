/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2014 Dominik Haumann <dhaumann@kde.org>
    Copyright (C) 2020 Christoph Cullmann <cullmann@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kateapp.h"
#include "katetabbar.h"

#include <QIcon>
#include <QMimeData>
#include <QPainter>
#include <QResizeEvent>
#include <QStyleOptionTab>
#include <QWheelEvent>

#include <KSharedConfig>
#include <KConfigGroup>

#include <KTextEditor/Document>

struct KateTabButtonData {
    KTextEditor::Document *doc = nullptr;
};

Q_DECLARE_METATYPE(KateTabButtonData)

/**
 * Creates a new tab bar with the given \a parent.
 */
KateTabBar::KateTabBar(QWidget *parent)
    : QTabBar(parent)
{
    // enable document mode, docs tell this will trigger:
    // On macOS this will look similar to the tabs in Safari or Sierra's Terminal.app.
    // this seems reasonable for our document tabs
    setDocumentMode(true);

    // we want drag and drop
    setAcceptDrops(true);

    // use as much size as possible for each tab
    setExpanding(true);

    // document close function should be there
    setTabsClosable(true);

    // allow users to re-arrange the tabs
    setMovable(true);

    // enforce configured limit
    readTabCountLimitConfig();

    // handle config changes
    connect(KateApp::self(), &KateApp::configurationChanged, this, &KateTabBar::readTabCountLimitConfig);
}

void KateTabBar::readTabCountLimitConfig()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, "General");

    // 0 == unlimited, normalized other inputs
    const int tabCountLimit = cgGeneral.readEntry("Tabbar Tab Limit", 0);
    m_tabCountLimit = (tabCountLimit <= 0) ? 0 : tabCountLimit;

    // use scroll buttons if we have no limit
    setUsesScrollButtons(m_tabCountLimit == 0);

    // elide if we have some limit
    setElideMode((m_tabCountLimit == 0) ? Qt::ElideNone : Qt::ElideMiddle);

    // if we enforce a limit: purge tabs that violate it
    if (m_tabCountLimit > 0 && (count() > m_tabCountLimit)) {
        // just purge last X tabs, this isn't that clever but happens only on config changes!
        while (count() > m_tabCountLimit) {
            removeTab(count() - 1);
        }
        setCurrentIndex(0);
    }
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
        setTabData(idx, QVariant::fromValue(KateTabButtonData {}));
    }
    return tabData(idx);
}

void KateTabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->accept();
    emit newTabRequested();
}

void KateTabBar::mousePressEvent(QMouseEvent *event)
{
    if (!isActive()) {
        emit activateViewSpaceRequested();
    }
    QTabBar::mousePressEvent(event);
}

void KateTabBar::contextMenuEvent(QContextMenuEvent *ev)
{
    int id = tabAt(ev->pos());
    if (id >= 0) {
        emit contextMenuRequest(id, ev->globalPos());
    }
}

void KateTabBar::wheelEvent(QWheelEvent *event)
{
    event->accept();

    // cycle through the tabs
    const int delta = event->angleDelta().x() + event->angleDelta().y();
    const int idx = (delta > 0) ? prevTab() : nextTab();
    setCurrentIndex(idx);
}

void KateTabBar::setTabDocument(int idx, KTextEditor::Document *doc)
{
    QVariant data = ensureValidTabData(idx);
    KateTabButtonData buttonData = data.value<KateTabButtonData>();
    buttonData.doc = doc;
    setTabData(idx, QVariant::fromValue(buttonData));
}

void KateTabBar::setCurrentDocument(KTextEditor::Document *doc)
{
    // in any case: update lru counter for this document, might add new element to hash
    // we have a tab after this call, too!
    m_docToLruCounterAndHasTab[doc] = std::make_pair(++m_lruCounter, true);

    // do we have a tab for this document?
    // if yes => just set as current one
    const int existingIndex = documentIdx(doc);
    if (existingIndex != -1) {
        setCurrentIndex(existingIndex);
        return;
    }

    // get right icon to use
    QIcon icon;
    if (doc->isModified()) {
        icon = QIcon::fromTheme(QStringLiteral("document-save"));
    }

    // else: if we are still inside the allowed number of tabs or have no limit
    // => create new tab and be done
    if ((m_tabCountLimit == 0) || count() < m_tabCountLimit) {
        m_beingAdded = doc;
        int inserted = insertTab(-1, doc->documentName());
        setTabIcon(inserted, icon);
        return;
    }

    // ok, we have already the limit of tabs reached:
    // replace the tab with the lowest lru counter => the least recently used

    // search for the right tab
    quint64 minCounter = static_cast<quint64>(-1);
    int indexToReplace = 0;
    KTextEditor::Document *docToReplace = nullptr;
    for (int idx = 0; idx < count(); idx++) {
        QVariant data = tabData(idx);
        if (!data.isValid()) {
            continue;
        }
        const quint64 currentCounter = m_docToLruCounterAndHasTab[data.value<KateTabButtonData>().doc].first;
        if (currentCounter <= minCounter) {
            minCounter = currentCounter;
            indexToReplace = idx;
            docToReplace = data.value<KateTabButtonData>().doc;
        }
    }

    // mark the replace doc as "has no tab"
    m_docToLruCounterAndHasTab[docToReplace].second = false;

    // replace it's data + set it as active
    setTabText(indexToReplace, doc->documentName());
    setTabDocument(indexToReplace, doc);
    setTabToolTip(indexToReplace, doc->url().toDisplayString());
    setTabIcon(indexToReplace, icon);
    setCurrentIndex(indexToReplace);
}

void KateTabBar::removeDocument(KTextEditor::Document *doc)
{
    // purge LRU storage, must work
    Q_ASSERT(m_docToLruCounterAndHasTab.erase(doc) == 1);

    // remove document if needed, we might have no tab for it, if tab count is limited!
    const int idx = documentIdx(doc);
    if (idx != -1) {
        // purge the tab we have
        removeTab(idx);

        // if we have some tab limit, replace the removed tab with the next best document that has none!
        if (m_tabCountLimit > 0) {
            quint64 maxCounter = 0;
            KTextEditor::Document *docToReplace = nullptr;
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

            // any document found? add tab for it
            if (docToReplace) {
                // get right icon to use
                QIcon icon;
                if (docToReplace->isModified()) {
                    icon = QIcon::fromTheme(QStringLiteral("document-save"));
                }

                m_beingAdded = docToReplace;
                int inserted = insertTab(idx, docToReplace->documentName());
                setTabIcon(inserted, icon);

                // mark the replace doc as "has a tab"
                m_docToLruCounterAndHasTab[docToReplace].second = true;
            }
        }
    }
}

int KateTabBar::documentIdx(KTextEditor::Document *doc)
{
    for (int idx = 0; idx < count(); idx++) {
        QVariant data = tabData(idx);
        if (!data.isValid()) {
            continue;
        }
        if (data.value<KateTabButtonData>().doc != doc) {
            continue;
        }
        return idx;
    }
    return -1;
}

KTextEditor::Document *KateTabBar::tabDocument(int idx)
{
    QVariant data = ensureValidTabData(idx);
    KateTabButtonData buttonData = data.value<KateTabButtonData>();

    KTextEditor::Document *doc = nullptr;
    // The tab got activated before the correct finalixation,
    // we need to plug the document before returning.
    if (buttonData.doc == nullptr && m_beingAdded) {
        setTabDocument(idx, m_beingAdded);
        doc = m_beingAdded;
        m_beingAdded = nullptr;
    } else {
        doc = buttonData.doc;
    }

    return doc;
}

void KateTabBar::tabInserted(int idx)
{
    if (m_beingAdded) {
        setTabDocument(idx, m_beingAdded);
    }
    setTabToolTip(idx, tabDocument(idx)->url().toDisplayString());
    m_beingAdded = nullptr;
}

QVector<KTextEditor::Document *> KateTabBar::documentList() const
{
    QVector<KTextEditor::Document *> result;
    for (int idx = 0; idx < count(); idx++) {
        QVariant data = tabData(idx);
        if (!data.isValid()) {
            continue;
        }
        result.append(data.value<KateTabButtonData>().doc);
    }
    return result;
}
