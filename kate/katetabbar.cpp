/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2014 Dominik Haumann <dhaumann@kde.org>

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

#include "katetabbar.h"

#include <math.h> // ceil

// #include <QDebug>
#include <QMimeData>
#include <QPainter>
#include <QResizeEvent>
#include <QStyleOptionTab>
#include <QWheelEvent>

#include <KTextEditor/Document>

struct KateTabButtonData {
    QUrl url;
    KTextEditor::Document *doc = nullptr;
};

Q_DECLARE_METATYPE(KateTabButtonData);

class KateTabBar::KateTabBarPrivate
{
public:
    // pointer to tabbar
    KateTabBar *q = nullptr;
    bool isActive = false;
    KTextEditor::Document *beingAdded;
};

/**
 * Creates a new tab bar with the given \a parent.
 */
KateTabBar::KateTabBar(QWidget *parent)
    : QTabBar(parent)
    , d(new KateTabBarPrivate())
{
    d->q = this;
    d->isActive = false;

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setAcceptDrops(true);
    setExpanding(false);
}

/**
 * Destroys the tab bar.
 */
KateTabBar::~KateTabBar()
{
    delete d;
}

void KateTabBar::setActive(bool active)
{
    if (active == d->isActive) {
        return;
    }
    d->isActive = active;
    update();
}

bool KateTabBar::isActive() const
{
    return d->isActive;
}

int KateTabBar::prevTab() const
{
    return currentIndex() == 0 ? count() - 1 // first index, wrap to last
         : currentIndex() - 1;
}

int KateTabBar::nextTab() const
{
    return currentIndex() == count() - 1 ? 0 // last index, wrap to first
         : currentIndex() + 1;
}

bool KateTabBar::containsTab(int index) const
{
    return index >= 0 && index < count();
}

QVariant KateTabBar::ensureValidTabData(int idx)
{
    if (!tabData(idx).isValid()) {
        setTabData(idx, QVariant::fromValue(KateTabButtonData{}));
    }
    return tabData(idx);
}

void KateTabBar::setTabUrl(int idx, const QUrl &url)
{
    QVariant data = ensureValidTabData(idx);
    KateTabButtonData buttonData = data.value<KateTabButtonData>();
    buttonData.url = url;
    setTabData(idx, QVariant::fromValue(buttonData));
}

QUrl KateTabBar::tabUrl(int idx)
{
    QVariant data = ensureValidTabData(idx);
    return data.value<KateTabButtonData>().url;
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

int KateTabBar::maxTabCount() const
{
    return 10; // TODO: Fix this.
}

void KateTabBar::setTabDocument(int idx, KTextEditor::Document* doc)
{

    QVariant data = ensureValidTabData(idx);
    KateTabButtonData buttonData = data.value<KateTabButtonData>();
    buttonData.doc = doc;
    setTabData(idx, QVariant::fromValue(buttonData));
}

void KateTabBar::setCurrentDocument(KTextEditor::Document *doc)
{
    setCurrentIndex(documentIdx(doc));
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
    if (buttonData.doc == nullptr && d->beingAdded) {
        setTabDocument(idx, d->beingAdded);
        doc = d->beingAdded;
        d->beingAdded = nullptr;
    } else {
        doc = buttonData.doc;
    }

    return doc;
}

int KateTabBar::insertTab(int idx, KTextEditor::Document* doc)
{
    d->beingAdded = doc;
    return insertTab(idx, doc->documentName());
}

void KateTabBar::tabInserted(int idx)
{
    if (d->beingAdded) {
        setTabDocument(idx, d->beingAdded);
    }
    setTabToolTip(idx, tabDocument(idx)->url().toDisplayString());
    d->beingAdded = nullptr;
}
