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

#include "math.h"

#include <QPainter>
#include <QResizeEvent>
#include <QStyleOptionTab>

/**
 * Creates a new tab bar with the given \a parent.
 */
KateTabBar::KateTabBar(QWidget *parent)
    : QWidget(parent)
{
    m_minimumTabWidth = 150;
    m_maximumTabWidth = 350;
    m_currentTabWidth = 350;
    m_keepTabWidth = false;

    m_isActiveViewSpace = false;

    m_nextID = 0;

    m_activeButton = Q_NULLPTR;

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

/**
 * Destroys the tab bar.
 */
KateTabBar::~KateTabBar()
{
}

void KateTabBar::setActiveViewSpace(bool active)
{
    if (active != m_isActiveViewSpace) {
        m_isActiveViewSpace = active;
        update();
    }
}

bool KateTabBar::isActiveViewSpace() const
{
    return m_isActiveViewSpace;
}

int KateTabBar::addTab(const QString &text)
{
    return insertTab(m_tabButtons.size(), text);
}

int KateTabBar::insertTab(int position, const QString & text)
{
    Q_ASSERT(position <= m_tabButtons.size());

    // -1 is append
    if (position < 0) {
        position = m_tabButtons.size();
    }

    KateTabButton *tabButton = new KateTabButton(text, this);

    m_tabButtons.insert(position, tabButton);
    m_idToTab[m_nextID] = tabButton;
    connect(tabButton, SIGNAL(activated(KateTabButton*)),
            this, SLOT(tabButtonActivated(KateTabButton*)));
    connect(tabButton, SIGNAL(closeRequest(KateTabButton*)),
            this, SLOT(tabButtonCloseRequest(KateTabButton*)));

    // abort potential keeping of width
    m_keepTabWidth = false;

    updateButtonPositions(true);

    return m_nextID++;
}

int KateTabBar::currentTab() const
{
    return m_idToTab.key(m_activeButton, -1);
}

void KateTabBar::setCurrentTab(int id)
{
    Q_ASSERT(m_idToTab.contains(id));

    KateTabButton *tabButton = m_idToTab[id];
    if (m_activeButton == tabButton) {
        return;
    }

    if (m_activeButton) {
        m_activeButton->setChecked(false);
    }

    m_activeButton = tabButton;
    m_activeButton->setChecked(true);
}

int KateTabBar::removeTab(int id)
{
    Q_ASSERT(m_idToTab.contains(id));

    KateTabButton *tabButton = m_idToTab[id];

    if (tabButton == m_activeButton) {
        m_activeButton = Q_NULLPTR;
    }

    const int position = m_tabButtons.indexOf(tabButton);

    m_idToTab.remove(id);
    m_tabButtons.removeAt(position);
    // delete the button with deleteLater() because the button itself might
    // have send a close-request. So the app-execution is still in the
    // button, a delete tabButton; would lead to a crash.
    tabButton->hide();
    tabButton->deleteLater();

    updateButtonPositions(true);

    return position;
}

bool KateTabBar::containsTab(int id) const
{
    return m_idToTab.contains(id);
}

void KateTabBar::setTabText(int id, const QString &text)
{
    Q_ASSERT(m_idToTab.contains(id));
    m_idToTab[id]->setText(text);
}

QString KateTabBar::tabText(int id) const
{
    Q_ASSERT(m_idToTab.contains(id));
    return m_idToTab[id]->text();
}

void KateTabBar::setTabToolTip(int id, const QString &tip)
{
    Q_ASSERT(m_idToTab.contains(id));
    m_idToTab[id]->setToolTip(tip);
}

QString KateTabBar::tabToolTip(int id) const
{
    Q_ASSERT(m_idToTab.contains(id));
    return m_idToTab[id]->toolTip();
}

void KateTabBar::setTabIcon(int id, const QIcon &icon)
{
    Q_ASSERT(m_idToTab.contains(id));
    m_idToTab[id]->setIcon(icon);
}

QIcon KateTabBar::tabIcon(int id) const
{
    Q_ASSERT(m_idToTab.contains(id));
    return m_idToTab[id]->icon();
}

int KateTabBar::count() const
{
    return m_tabButtons.count();
}

void KateTabBar::tabButtonActivated(KateTabButton *tabButton)
{
    if (tabButton == m_activeButton) {
        // make sure we are the currently active view space
        if (! isActiveViewSpace()) {
            emit activateViewSpaceRequested();
        }
        return;
    }

    if (m_activeButton) {
        m_activeButton->setChecked(false);
    }

    m_activeButton = tabButton;
    m_activeButton->setChecked(true);

    const int id = m_idToTab.key(m_activeButton, -1);
    Q_ASSERT(id >= 0);
    emit currentChanged(id);
}

void KateTabBar::tabButtonCloseRequest(KateTabButton *tabButton)
{
    const int id = m_idToTab.key(tabButton, -1);
    Q_ASSERT(id >= 0);

    // keep width
    if (underMouse()) {
        m_keepTabWidth = true;
    }

    emit closeTabRequested(id);
}

void KateTabBar::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)

    // fix button positions
    if (!m_keepTabWidth || event->size().width() < event->oldSize().width()) {
        updateButtonPositions();
    }

    const int tabDiff = maxTabCount() - m_tabButtons.size();
    if (tabDiff > 0) {
        emit moreTabsRequested(tabDiff);
    } else if (tabDiff < 0) {
        emit lessTabsRequested(-tabDiff);
    }
}

void KateTabBar::updateButtonPositions(bool animate)
{
    // if there are no tabs there is nothing to do
    if (m_tabButtons.count() == 0) {
        return;
    }

    const int barWidth = width();
    const int maxCount = maxTabCount();

    // how many tabs do we show?
    const int visibleTabCount = qMin(count(), maxCount);

    // new tab width of each tab
    qreal tabWidth;
    const bool keepWidth = m_keepTabWidth && ceil(m_currentTabWidth) * visibleTabCount < barWidth;
    if (keepWidth) {
        // only keep tab width if the tabs still fit
        tabWidth = m_currentTabWidth;
    } else {
        tabWidth = qMin(static_cast<qreal>(barWidth) / visibleTabCount, static_cast<qreal>(m_maximumTabWidth));
    }

    // if the last tab was closed through the close button, make sure the
    // close button of the new tab is again under the mouse
    if (keepWidth) {
        const int xPos = mapFromGlobal(QCursor::pos()).x();
        if (tabWidth * visibleTabCount < xPos) {
            tabWidth = qMin(tabWidth * (visibleTabCount + 1.0) / visibleTabCount, static_cast<qreal>(m_maximumTabWidth));
        }
    }

    // now save the current tab width for future adaptation
    m_currentTabWidth = tabWidth;

    // now set the sizes
    const int maxi = m_tabButtons.size();
    const int w = ceil(tabWidth);
    const int h = height();
    for (int i = 0; i < maxi; ++i) {
        KateTabButton *tabButton = m_tabButtons.value(i);
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

int KateTabBar::maxTabCount() const
{
    return qMax(1, width() / m_minimumTabWidth);
}

void KateTabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->accept();
    emit newTabRequested();
}

void KateTabBar::mousePressEvent(QMouseEvent *event)
{
    if (! isActiveViewSpace()) {
        emit activateViewSpaceRequested();
    }
    QWidget::mousePressEvent(event);
}

void KateTabBar::leaveEvent(QEvent *event) 
{
    if (m_keepTabWidth) {
        m_keepTabWidth = false;
        updateButtonPositions(true);
    }

    QWidget::leaveEvent(event);
}

void KateTabBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const int buttonCount = m_tabButtons.size();
    if (buttonCount < 1) {
        return;
    }

    // draw separators
    QStyleOption option;
    option.initFrom(m_tabButtons[0]);
    option.state |= QStyle::State_Horizontal;
    const int w = style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent, 0, this);
    const int offset = w / 2;
    option.rect.setWidth(w);
    option.rect.moveTop(0);

    QPainter painter(this);

    // first separator
    option.rect.moveLeft(m_tabButtons[0]->geometry().left() - offset);
    style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &option, &painter);

    // all other separators
    for (int i = 0; i < buttonCount; ++i) {
        option.rect.moveLeft(m_tabButtons[i]->geometry().right() - offset);
        style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &option, &painter);
    }
}

void KateTabBar::contextMenuEvent(QContextMenuEvent *ev)
{
    int id = -1;
    foreach (KateTabButton * button, m_tabButtons) {
        if (button->rect().contains(button->mapFromGlobal(ev->globalPos()))) {
            id = m_idToTab.key(button, -1);
            Q_ASSERT(id >= 0);
            break;
        }
    }

    emit contextMenuRequest(id, ev->globalPos());
}
