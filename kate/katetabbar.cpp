/*   This file is part of the KDE project
 *
 *   Copyright (C) 2014 Dominik Haumann <dhaumann@kde.org>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public License
 *   along with this library; see the file COPYING.LIB.  If not, write to
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *   Boston, MA 02110-1301, USA.
 */

#include "katetabbar.h"
#include "katetabbutton.h"

#include <math.h> // ceil

// #include <QDebug>
#include <QMimeData>
#include <QPainter>
#include <QResizeEvent>
#include <QStyleOptionTab>
#include <QWheelEvent>

class KateTabBarPrivate
{
public:
    // pointer to tabbar
    KateTabBar *q;

    // minimum and maximum tab width
    int minimumTabWidth;
    int maximumTabWidth;

    // current tab width: when closing tabs with the mouse, we keep
    // the tab width fixed until the mouse leaves the tab bar. This
    // way the user can keep clicking the close button without moving
    // the ouse.
    qreal currentTabWidth;
    bool keepTabWidth;

    bool isActive;

    QVector<KateTabButton *> tabButtons;
    QHash<int, KateTabButton *> idToTab;

    KateTabButton *activeButton;

    int nextID;

public: // functions
    /**
     * Set tab geometry. The tabs are animated only if @p animate is @e true.
     */
    void updateButtonPositions(bool animate = false)
    {
        // if there are no tabs there is nothing to do
        if (tabButtons.count() == 0) {
            return;
        }

        // check whether an animation is still running, in that case,
        // continue animations to avoid jumping tabs
        const int maxi = tabButtons.size();
        if (!animate) {
            for (int i = 0; i < maxi; ++i) {
                if (tabButtons.value(i)->geometryAnimationRunning()) {
                    animate = true;
                    break;
                }
            }
        }

        const int barWidth = q->width();
        const int maxCount = q->maxTabCount();

        // how many tabs do we show?
        const int visibleTabCount = qMin(q->count(), maxCount);

        // new tab width of each tab
        qreal tabWidth;
        const bool keepWidth = keepTabWidth && ceil(currentTabWidth) * visibleTabCount < barWidth;
        if (keepWidth) {
            // only keep tab width if the tabs still fit
            tabWidth = currentTabWidth;
        } else {
            tabWidth = qMin(static_cast<qreal>(barWidth) / visibleTabCount, static_cast<qreal>(maximumTabWidth));
        }

        // if the last tab was closed through the close button, make sure the
        // close button of the new tab is again under the mouse
        if (keepWidth) {
            const int xPos = q->mapFromGlobal(QCursor::pos()).x();
            if (tabWidth * visibleTabCount < xPos) {
                tabWidth = qMin(static_cast<qreal>(tabWidth * (visibleTabCount + 1.0)) / visibleTabCount, static_cast<qreal>(maximumTabWidth));
            }
        }

        // now save the current tab width for future adaptation
        currentTabWidth = tabWidth;

        // now set the sizes
        const int w = ceil(tabWidth);
        const int h = q->height();
        for (int i = 0; i < maxi; ++i) {
            KateTabButton *tabButton = tabButtons.value(i);
            if (i >= maxCount) {
                tabButton->hide();
            } else {
                QRect endGeometry(i * tabWidth, 0, w, h);
                if (i > 0) {
                    // make sure the tab button starts exactly next to the previous tab (avoid rounding errors)
                    endGeometry.setLeft((i-1) * tabWidth + w);
                }

                if (animate) {
                    const QRect startGeometry = tabButton->isVisible() ? tabButton->geometry()
                    : QRect(i * tabWidth, 0, 0, h);
                    tabButton->setAnimatedGeometry(startGeometry, endGeometry);
                } else {
                    // two times endGeometry. Takes care of stopping a running animation
                    tabButton->setAnimatedGeometry(endGeometry, endGeometry);
                }
                tabButton->show();
            }
        }
    }
};

/**
 * Creates a new tab bar with the given \a parent.
 */
KateTabBar::KateTabBar(QWidget *parent)
    : QWidget(parent)
    , d(new KateTabBarPrivate())
{
    d->q = this;
    d->minimumTabWidth = 150;
    d->maximumTabWidth = 350;
    d->currentTabWidth = 350;
    d->keepTabWidth = false;

    d->isActive = false;

    d->activeButton = nullptr;

    d->nextID = 0;

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setAcceptDrops(true);
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
    if (active != d->isActive) {
        d->isActive = active;
        update();
    }
}

bool KateTabBar::isActive() const
{
    return d->isActive;
}

int KateTabBar::addTab(const QString &text)
{
    return insertTab(d->tabButtons.size(), text);
}

int KateTabBar::insertTab(int position, const QString & text)
{
    Q_ASSERT(position <= d->tabButtons.size());

    // -1 is append
    if (position < 0) {
        position = d->tabButtons.size();
    }

    KateTabButton *tabButton = new KateTabButton(text, this);

    d->tabButtons.insert(position, tabButton);
    d->idToTab[d->nextID] = tabButton;
    connect(tabButton, &KateTabButton::activated, this, &KateTabBar::tabButtonActivated);
    connect(tabButton, &KateTabButton::closeRequest, this, &KateTabBar::tabButtonCloseRequest);

    // abort potential keeping of width
    d->keepTabWidth = false;

    d->updateButtonPositions(true);

    return d->nextID++;
}

int KateTabBar::currentTab() const
{
    return d->idToTab.key(d->activeButton, -1);
}

void KateTabBar::setCurrentTab(int id)
{
    Q_ASSERT(d->idToTab.contains(id));

    KateTabButton *tabButton = d->idToTab.value(id, nullptr);
    if (d->activeButton == tabButton) {
        return;
    }

    if (d->activeButton) {
        d->activeButton->setChecked(false);
    }

    d->activeButton = tabButton;
    if (d->activeButton) {
        d->activeButton->setChecked(true);
    }
}

int KateTabBar::prevTab() const
{
    const int curId = currentTab();

    if (curId >= 0) {
        KateTabButton *tabButton = d->idToTab.value(curId, nullptr);
        const int index = d->tabButtons.indexOf(tabButton);
        Q_ASSERT(index >= 0);

        if (index > 0) {
            return d->idToTab.key(d->tabButtons[index - 1], -1);
        } else if (count() > 1) {
            // cycle through tabbar
            return d->idToTab.key(d->tabButtons.last(), -1);
        }
    }

    return -1;
}

int KateTabBar::nextTab() const
{
    const int curId = currentTab();

    if (curId >= 0) {
        KateTabButton *tabButton = d->idToTab.value(curId, nullptr);
        const int index = d->tabButtons.indexOf(tabButton);
        Q_ASSERT(index >= 0);

        if (index < d->tabButtons.size() - 1) {
            return d->idToTab.key(d->tabButtons[index + 1], -1);
        } else if (count() > 1) {
            // cycle through tabbar
            return d->idToTab.key(d->tabButtons.first(), -1);
        }
    }

    return -1;
}

int KateTabBar::removeTab(int id)
{
    Q_ASSERT(d->idToTab.contains(id));

    KateTabButton *tabButton = d->idToTab.value(id, nullptr);

    if (tabButton == d->activeButton) {
        d->activeButton = Q_NULLPTR;
    }

    const int position = d->tabButtons.indexOf(tabButton);

    if (position != -1) {
        d->idToTab.remove(id);
        d->tabButtons.removeAt(position);
    }

    if (tabButton) {
        // delete the button with deleteLater() because the button itself might
        // have send a close-request. So the app-execution is still in the
        // button, a delete tabButton; would lead to a crash.
        tabButton->hide();
        tabButton->deleteLater();
    }

    d->updateButtonPositions(true);

    return position;
}

bool KateTabBar::containsTab(int id) const
{
    return d->idToTab.contains(id);
}

void KateTabBar::setTabText(int id, const QString &text)
{
    Q_ASSERT(d->idToTab.contains(id));
    KateTabButton * tabButton = d->idToTab.value(id, nullptr);
    if (tabButton) {
        tabButton->setText(text);
    }
}

QString KateTabBar::tabText(int id) const
{
    Q_ASSERT(d->idToTab.contains(id));
    KateTabButton * tabButton = d->idToTab.value(id, nullptr);
    if (tabButton) {
        return tabButton->text();
    }
    return QString();
}

void KateTabBar::setTabToolTip(int id, const QString &tip)
{
    Q_ASSERT(d->idToTab.contains(id));
    KateTabButton * tabButton = d->idToTab.value(id, nullptr);
    if (tabButton) {
        tabButton->setToolTip(tip);
    }
}

QString KateTabBar::tabToolTip(int id) const
{
    Q_ASSERT(d->idToTab.contains(id));
    KateTabButton * tabButton = d->idToTab.value(id, nullptr);
    if (tabButton) {
        return tabButton->toolTip();
    }
    return QString();
}

void KateTabBar::setTabUrl(int id, const QUrl &url)
{
    Q_ASSERT(d->idToTab.contains(id));
    KateTabButton * tabButton = d->idToTab.value(id, nullptr);
    if (tabButton) {
        tabButton->setUrl(url);
    }
}

QUrl KateTabBar::tabUrl(int id) const
{
    Q_ASSERT(d->idToTab.contains(id));
    KateTabButton * tabButton = d->idToTab.value(id, nullptr);
    if (tabButton) {
        return tabButton->url();
    }
    return QUrl();
}

void KateTabBar::setTabIcon(int id, const QIcon &icon)
{
    Q_ASSERT(d->idToTab.contains(id));
    KateTabButton * tabButton = d->idToTab.value(id, nullptr);
    if (tabButton) {
        tabButton->setIcon(icon);
    }
}

QIcon KateTabBar::tabIcon(int id) const
{
    Q_ASSERT(d->idToTab.contains(id));
    KateTabButton * tabButton = d->idToTab.value(id, nullptr);
    if (tabButton) {
        return tabButton->icon();
    }
    return QIcon();
}

int KateTabBar::count() const
{
    return d->tabButtons.count();
}

void KateTabBar::tabButtonActivated(KateTabButton *tabButton)
{
    if (!tabButton) {
        return;
    }

    if (tabButton == d->activeButton) {
        // make sure we are the currently active view space
        if (! isActive()) {
            emit activateViewSpaceRequested();
        }
        return;
    }

    if (d->activeButton) {
        d->activeButton->setChecked(false);
    }

    d->activeButton = tabButton;
    d->activeButton->setChecked(true);

    const int id = d->idToTab.key(d->activeButton, -1);
    Q_ASSERT(id >= 0);
    if (id >= 0) {
        emit currentChanged(id);
    }
}

void KateTabBar::tabButtonCloseRequest(KateTabButton *tabButton)
{
    // keep width
    if (underMouse()) {
        d->keepTabWidth = true;
    }

    const int id = d->idToTab.key(tabButton, -1);
    Q_ASSERT(id >= 0);
    if (id >= 0) {
        emit closeTabRequested(id);
    }
}

void KateTabBar::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)

    // fix button positions
    if (!d->keepTabWidth || event->size().width() < event->oldSize().width()) {
        d->updateButtonPositions();
    }

    const int tabDiff = maxTabCount() - d->tabButtons.size();
    if (tabDiff > 0) {
        emit moreTabsRequested(tabDiff);
    } else if (tabDiff < 0) {
        emit lessTabsRequested(-tabDiff);
    }
}

int KateTabBar::maxTabCount() const
{
    return qMax(1, width() / d->minimumTabWidth);
}

void KateTabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->accept();
    emit newTabRequested();
}

void KateTabBar::mousePressEvent(QMouseEvent *event)
{
    if (! isActive()) {
        emit activateViewSpaceRequested();
    }
    QWidget::mousePressEvent(event);
}

void KateTabBar::leaveEvent(QEvent *event) 
{
    if (d->keepTabWidth) {
        d->keepTabWidth = false;
        d->updateButtonPositions(true);
    }

    QWidget::leaveEvent(event);
}

void KateTabBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const int buttonCount = d->tabButtons.size();
    if (buttonCount < 1) {
        return;
    }

    // draw separators
    QStyleOption option;
    option.initFrom(d->tabButtons[0]);
    option.state |= QStyle::State_Horizontal;
    const int w = style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent, nullptr, this);
    const int offset = w / 2;
    option.rect.setWidth(w);
    option.rect.moveTop(0);

    QPainter painter(this);

    // first separator
    option.rect.moveLeft(d->tabButtons[0]->geometry().left() - offset);
    style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &option, &painter);

    // all other separators
    for (int i = 0; i < buttonCount; ++i) {
        option.rect.moveLeft(d->tabButtons[i]->geometry().right() - offset);
        style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &option, &painter);
    }
}

void KateTabBar::contextMenuEvent(QContextMenuEvent *ev)
{
    int id = -1;
    foreach (KateTabButton * button, d->tabButtons) {
        if (button->rect().contains(button->mapFromGlobal(ev->globalPos()))) {
            id = d->idToTab.key(button, -1);
            Q_ASSERT(id >= 0);
            break;
        }
    }

    if (id >= 0) {
        emit contextMenuRequest(id, ev->globalPos());
    }
}

void KateTabBar::wheelEvent(QWheelEvent * event)
{
    event->accept();

    // cycle through the tabs
    const int delta = event->angleDelta().x() + event->angleDelta().y();
    const int id = (delta > 0) ? prevTab() : nextTab();
    if (id >= 0) {
        Q_ASSERT(d->idToTab.contains(id));
        KateTabButton *tabButton = d->idToTab.value(id, nullptr);
        if (tabButton) {
            tabButtonActivated(tabButton);
        }
    }
}

void KateTabBar::dragEnterEvent(QDragEnterEvent *event)
{
    const bool sameApplication = event->source() != nullptr;
    if (sameApplication && event->mimeData()->hasFormat(QStringLiteral("application/x-dndkatetabbutton"))) {
        if (event->source()->parent() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            // possibly dragged from another tabbar (not yet implemented)
            event->ignore();
        }
    } else {
        event->ignore();
    }
}

void KateTabBar::dragMoveEvent(QDragMoveEvent *event)
{
    // first of all, make sure the dragged button is from this tabbar
    auto button = qobject_cast<KateTabButton *>(event->source());
    if (!button) {
        event->ignore();
        return;
    }

    // save old tab position
    const int oldButtonIndex = d->tabButtons.indexOf(button);
    d->tabButtons.removeAt(oldButtonIndex);

    // find new position
    const qreal currentPos = event->pos().x();
    int index = d->tabButtons.size();
    for (int i = 0; i < d->tabButtons.size(); ++i) {
        auto tab = d->tabButtons[i];
        if (tab->geometry().center().x() > currentPos) {
            index = i;
            break;
        }
    }
    d->tabButtons.insert(index, button);

    // trigger rearrange if required
    if (oldButtonIndex != index) {
        d->updateButtonPositions(true);
    }

    event->accept();
}

void KateTabBar::dropEvent(QDropEvent *event)
{
    // no action required
    event->acceptProposedAction();
}
