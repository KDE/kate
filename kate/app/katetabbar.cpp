/*   This file is part of the KDE project
 *
 *   Copyright (C) 2014 Dominik Haumann <dhauumann@kde.org>
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

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kiconloader.h>
#include <kstringhandler.h>
#include <KLocalizedString>

#include <QToolButton>
#include <QApplication> // QApplication::sendEvent
#include <QtAlgorithms> // qSort
#include <QDebug>

KateTabBar::SortType global_sortType;

// public operator < for two tab buttons
bool tabLessThan(const KateTabButton *a, const KateTabButton *b)
{
    switch (global_sortType) {
    case KateTabBar::OpeningOrder: {
        return a->buttonID() < b->buttonID();
    }

    case KateTabBar::Name: {
        // fall back to ID
        if (a->text().toLower() == b->text().toLower()) {
            return a->buttonID() < b->buttonID();
        }

        return a->text() < b->text();
//             return KStringHandler::naturalCompare(a->text(), b->text(), Qt::CaseInsensitive) < 0; // FIXME KF5
    }

    case KateTabBar::Extension: {
        // sort by extension, but check whether the files have an
        // extension first
        const int apos = a->text().lastIndexOf(QLatin1Char('.'));
        const int bpos = b->text().lastIndexOf(QLatin1Char('.'));

        if (apos == -1 && bpos == -1) {
            return a->text().toLower() < b->text().toLower();
        } else if (apos == -1) {
            return true;
        } else if (bpos == -1) {
            return false;
        } else {
            const int aright = a->text().size() - apos;
            const int bright = b->text().size() - bpos;
            QString aExt = a->text().right(aright).toLower();
            QString bExt = b->text().right(bright).toLower();
            QString aFile = a->text().left(apos).toLower();
            QString bFile = b->text().left(bpos).toLower();

            if (aExt == bExt)
                return (aFile == bFile) ? a->buttonID() < b->buttonID()
                       : aFile < bFile;
            else {
                return aExt < bExt;
            }
        }
    }
    }

    return true;
}

/**
 * Creates a new tab bar with the given \a parent and \a name.
 *  .
 */
KateTabBar::KateTabBar(QWidget *parent)
    : QWidget(parent)
{
    m_minimumTabWidth = 150;
    m_maximumTabWidth = 350;

    // fixme: better additional size
    m_tabHeight = QFontMetrics(font()).height() + 10;

    m_sortType = OpeningOrder;
    m_nextID = 0;

    m_activeButton = 0L;

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    updateFixedHeight();
}

/**
 * Destroys the tab bar.
 */
KateTabBar::~KateTabBar()
{
}

/**
 * Loads the settings from \a config from section \a group.
 * Remembered properties are:
 *  - minimum and maximum tab width
 *  - fixed tab height
 *  - button colors
 *  - much more!
 *  .
 * The original group is saved and restored at the end of this function.
 *
 * \note Call @p load() immediately after you created the tabbar, otherwise
 *       some properties might not be restored correctly (like highlighted
 *       buttons).
 */
void KateTabBar::load(KConfigBase *config, const QString &group)
{
    KConfigGroup cg(config, group);

    // tabbar properties
    setMinimumTabWidth(cg.readEntry("minimum width", m_minimumTabWidth));
    setMaximumTabWidth(cg.readEntry("maximum width", m_maximumTabWidth));
    setTabSortType((SortType) cg.readEntry("sort type", (int)OpeningOrder));

    // highlighted entries
    QStringList documents = cg.readEntry("highlighted documents", QStringList());
    QStringList colors = cg.readEntry("highlighted colors", QStringList());

    // restore highlight map
    m_highlightedTabs.clear();
    for (int i = 0; i < documents.size() && i < colors.size(); ++i) {
        m_highlightedTabs[documents[i]] = colors[i];
    }

    setHighlightMarks(highlightMarks());
}

/**
 * Saves the settings to \a config into section \a group.
 * The original group is saved and restored at the end of this function.
 * See @p load() for more information.
 */
void KateTabBar::save(KConfigBase *config, const QString &group) const
{
    KConfigGroup cg(config, group);

    // tabbar properties
    cg.writeEntry("minimum width", minimumTabWidth());
    cg.writeEntry("maximum width", maximumTabWidth());
    cg.writeEntry("sort type", (int)tabSortType());

    // highlighted entries
    cg.writeEntry("highlighted documents", m_highlightedTabs.keys());
    cg.writeEntry("highlighted colors", m_highlightedTabs.values());
}

/**
 * Set the minimum width in pixels a tab must have.
 */
void KateTabBar::setMinimumTabWidth(int min_pixel)
{
    if (m_minimumTabWidth == min_pixel) {
        return;
    }

    m_minimumTabWidth = min_pixel;
    triggerResizeEvent();
}

/**
 * Set the maximum width in pixels a tab may have.
 */
void KateTabBar::setMaximumTabWidth(int max_pixel)
{
    if (m_maximumTabWidth == max_pixel) {
        return;
    }

    m_maximumTabWidth = max_pixel;
    triggerResizeEvent();
}

/**
 * Get the minimum width in pixels a tab can have.
 */
int KateTabBar::minimumTabWidth() const
{
    return m_minimumTabWidth;
}

/**
 * Get the maximum width in pixels a tab can have.
 */
int KateTabBar::maximumTabWidth() const
{
    return m_maximumTabWidth;
}

/**
 * Adds a new tab with text \a text. Returns the new tab's ID.
 */
int KateTabBar::addTab(const QString &text)
{
    return addTab(QIcon(), text);
}

/**
 * This is an overloaded member function, provided for convenience.
 * It behaves essentially like the above function.
 *
 * Adds a new tab with \a icon and \a text. Returns the new tab's index.
 */
int KateTabBar::addTab(const QIcon &icon, const QString &text)
{
    KateTabButton *tabButton = new KateTabButton(text, m_nextID, this);
    tabButton->setIcon(icon);
    if (m_highlightedTabs.contains(text)) {
        tabButton->setHighlightColor(QColor(m_highlightedTabs[text]));
    }

    m_tabButtons.append(tabButton);
    m_IDToTabButton[m_nextID] = tabButton;
    connect(tabButton, SIGNAL(activated(KateTabButton*)),
            this, SLOT(tabButtonActivated(KateTabButton*)));
    connect(tabButton, SIGNAL(highlightChanged(KateTabButton*)),
            this, SLOT(tabButtonHighlightChanged(KateTabButton*)));
    connect(tabButton, SIGNAL(closeRequest(KateTabButton*)),
            this, SLOT(tabButtonCloseRequest(KateTabButton*)));
    connect(tabButton, SIGNAL(closeOtherTabsRequest(KateTabButton*)),
            this, SLOT(tabButtonCloseOtherRequest(KateTabButton*)));
    connect(tabButton, SIGNAL(closeAllTabsRequest()),
            this, SLOT(tabButtonCloseAllRequest()));

    if (!isVisible()) {
        show();
    }

    updateSort();

    return m_nextID++;
}

void KateTabBar::raiseTab(int buttonId)
{
    Q_ASSERT(m_IDToTabButton.contains(buttonId));

    KateTabButton *button = m_IDToTabButton[buttonId];
    int tabIndex = m_tabButtons.indexOf(button);
    Q_ASSERT(tabIndex > -1);

    m_tabButtons.move(tabIndex, 0);

    triggerResizeEvent();
}

/**
 * Get the ID of the tab bar's activated tab. Returns -1 if no tab is activated.
 */
int KateTabBar::currentTab() const
{
    if (m_activeButton != 0L) {
        return m_activeButton->buttonID();
    }

    return -1;
}

/**
 * Activate the tab with ID \a index. No signal is emitted.
 */
void KateTabBar::setCurrentTab(int index)
{
    if (!m_IDToTabButton.contains(index)) {
        return;
    }

    KateTabButton *tabButton = m_IDToTabButton[index];
    if (m_activeButton == tabButton) {
        return;
    }

    if (m_activeButton) {
        m_activeButton->setActivated(false);
    }

    m_activeButton = tabButton;
    m_activeButton->setActivated(true);
}

/**
 * Removes the tab with ID \a index.
 */
void KateTabBar::removeTab(int index)
{
    if (!m_IDToTabButton.contains(index)) {
        return;
    }

    KateTabButton *tabButton = m_IDToTabButton[index];

    if (tabButton == m_activeButton) {
        m_activeButton = 0L;
    }

    m_IDToTabButton.remove(index);
    m_tabButtons.removeAll(tabButton);
    // delete the button with deleteLater() because the button itself might
    // have send a close-request. So the app-execution is still in the
    // button, a delete tabButton; would lead to a crash.
    tabButton->hide();
    tabButton->deleteLater();

    if (m_tabButtons.count() == 0) {
        hide();
    }

    triggerResizeEvent();
}

/**
 * Returns whether a tab with ID \a index exists.
 */
bool KateTabBar::containsTab(int index) const
{
    return m_IDToTabButton.contains(index);
}

/**
 * Sets the text of the tab with ID \a index to \a text.
 * \see tabText()
 */
void KateTabBar::setTabText(int index, const QString &text)
{
    if (!m_IDToTabButton.contains(index)) {
        return;
    }

    // change highlight key, if entry exists
    if (m_highlightedTabs.contains(m_IDToTabButton[index]->text())) {
        QString value = m_highlightedTabs[m_IDToTabButton[index]->text()];
        m_highlightedTabs.remove(m_IDToTabButton[index]->text());
        m_highlightedTabs[text] = value;

        // do not emit highlightMarksChanged(), because every tabbar gets this
        // change (usually)
        // emit highlightMarksChanged( this );
    }

    m_IDToTabButton[index]->setText(text);

    if (tabSortType() == Name || tabSortType() == Extension) {
        updateSort();
    }
}

/**
 * Returns the text of the tab with ID \a index. If the button id does not
 * exist \a QString() is returned.
 * \see setTabText()
 */
QString KateTabBar::tabText(int index) const
{
    if (m_IDToTabButton.contains(index)) {
        return m_IDToTabButton[index]->text();
    }

    return QString();
}

/**
 * Set the button @p index's tool tip to @p tip.
 */
void KateTabBar::setTabToolTip(int index, const QString &tip)
{
    if (!m_IDToTabButton.contains(index)) {
        return;
    }

    m_IDToTabButton[index]->setToolTip(tip);
}

/**
 * Get the button @p index's url. Result is QStrint() if not available.
 */
QString KateTabBar::tabToolTip(int index) const
{
    if (m_IDToTabButton.contains(index)) {
        return m_IDToTabButton[index]->toolTip();
    }

    return QString();
}

/**
 * Sets the icon of the tab with ID \a index to \a icon.
 * \see tabIcon()
 */
void KateTabBar::setTabIcon(int index, const QIcon &icon)
{
    if (m_IDToTabButton.contains(index)) {
        m_IDToTabButton[index]->setIcon(icon);
    }
}

/**
 * Returns the icon of the tab with ID \a index. If the button id does not
 * exist \a QIcon() is returned.
 * \see setTabIcon()
 */
QIcon KateTabBar::tabIcon(int index) const
{
    if (m_IDToTabButton.contains(index)) {
        return m_IDToTabButton[index]->icon();
    }

    return QIcon();
}

/**
 * Returns the number of tabs in the tab bar.
 */
int KateTabBar::count() const
{
    return m_tabButtons.count();
}

/**
 * Set the sort tye to @p sort.
 */
void KateTabBar::setTabSortType(SortType sort)
{
    if (m_sortType == sort) {
        return;
    }

    m_sortType = sort;
    updateSort();
}

/**
 * Get the sort type.
 */
KateTabBar::SortType KateTabBar::tabSortType() const
{
    return m_sortType;
}

void KateTabBar::setTabModified(int index, bool modified)
{
    if (m_IDToTabButton.contains(index)) {
        m_IDToTabButton[index]->setModified(modified);
    }
}

bool KateTabBar::isTabModified(int index) const
{
    if (m_IDToTabButton.contains(index)) {
        return m_IDToTabButton[index]->isModified();
    }

    return false;
}

void KateTabBar::removeHighlightMarks()
{
    KateTabButton *tabButton;
    foreach(tabButton, m_tabButtons) {
        if (tabButton->highlightColor().isValid()) {
            tabButton->setHighlightColor(QColor());
        }
    }

    m_highlightedTabs.clear();
    emit highlightMarksChanged(this);
}

void KateTabBar::setHighlightMarks(const QMap<QString, QString> &marks)
{
    m_highlightedTabs = marks;

    KateTabButton *tabButton;
    foreach(tabButton, m_tabButtons) {
        if (marks.contains(tabButton->text())) {
            if (tabButton->highlightColor().name() != marks[tabButton->text()]) {
                tabButton->setHighlightColor(QColor(marks[tabButton->text()]));
            }
        } else if (tabButton->highlightColor().isValid()) {
            tabButton->setHighlightColor(QColor());
        }
    }
}

QMap<QString, QString> KateTabBar::highlightMarks() const
{
    return m_highlightedTabs;
}

/**
 * Active button changed. Emit signal \p currentChanged() with the button's ID.
 */
void KateTabBar::tabButtonActivated(KateTabButton *tabButton)
{
    if (tabButton == m_activeButton) {
        return;
    }

    if (m_activeButton) {
        m_activeButton->setActivated(false);
    }

    m_activeButton = tabButton;
    m_activeButton->setActivated(true);

    emit currentChanged(tabButton->buttonID());
}

/**
 * The \e tabButton's highlight color changed, so update the list of documents
 * and colors.
 */
void KateTabBar::tabButtonHighlightChanged(KateTabButton *tabButton)
{
    if (tabButton->highlightColor().isValid()) {
        m_highlightedTabs[tabButton->text()] = tabButton->highlightColor().name();
        emit highlightMarksChanged(this);
    } else if (m_highlightedTabs.contains(tabButton->text())) {
        // invalid color, so remove the item
        m_highlightedTabs.remove(tabButton->text());
        emit highlightMarksChanged(this);
    }
}

/**
 * If the user wants to close a tab with the context menu, it sends a close
 * request. Throw the close request by emitting the signal @p closeRequest().
 */
void KateTabBar::tabButtonCloseRequest(KateTabButton *tabButton)
{
    emit tabCloseRequested(tabButton->buttonID());
}

/**
 * If the user wants to close all tabs except the current one using the context
 * menu, it sends multiple close requests.
 * Throw the close requests by emitting the signal @p closeRequest().
 */
void KateTabBar::tabButtonCloseOtherRequest(KateTabButton *tabButton)
{
    QList <int> tabToCloseID;
    for (int i = 0; i < m_tabButtons.size(); ++i) {
        if ((m_tabButtons.at(i))->buttonID() != tabButton->buttonID()) {
            tabToCloseID << (m_tabButtons.at(i))->buttonID();
        }
    }

    for (int i = 0; i < tabToCloseID.size(); i++) {
        emit tabCloseRequested(tabToCloseID.at(i));
    }
}

/**
 * If the user wants to close all the tabs using the context menu, it sends
 * multiple close requests.
 * Throw the close requests by emitting the signal @p closeRequest().
 */
void KateTabBar::tabButtonCloseAllRequest()
{
    QList <int> tabToCloseID;
    for (int i = 0; i < m_tabButtons.size(); ++i) {
        tabToCloseID << (m_tabButtons.at(i))->buttonID();
    }

    for (int i = 0; i < tabToCloseID.size(); i++) {
        emit tabCloseRequested(tabToCloseID.at(i));
    }
}

/**
 * Recalculate geometry for all children.
 */
void KateTabBar::resizeEvent(QResizeEvent *event)
{
    // if there are no tabs there is nothing to do
    if (m_tabButtons.count() == 0) {
        return;
    }

    int barWidth = event->size().width();
    const int maxCount = maxTabCount();

    // how many tabs do we show?
    const int visibleTabCount = qMin(count(), maxCount);

    // new tab width of each tab
    const qreal tabWidth = qMin(static_cast<qreal>(barWidth) / visibleTabCount,
                                static_cast<qreal>(m_maximumTabWidth));

    // now set the sizes
    const int maxi = m_tabButtons.size();
    for (int i = 0; i < maxi; ++i) {
        KateTabButton *tabButton = m_tabButtons.value(i);
        if (i >= maxCount) {
            tabButton->hide();
        } else {
            const int w = ceil(tabWidth);
            tabButton->setGeometry(i * w, 0, w, m_tabHeight);
            tabButton->show();
        }
    }

//     if (visibleTabCount
}

/**
 * Return the maximum amount of tabs that fit into the tab bar given
 * the minimumTabWidth().
 */
int KateTabBar::maxTabCount() const
{
    return qMax(1, width() / minimumTabWidth());
}

/**
 * Updates the fixed height. Called when the tab height or the number of rows
 * changed.
 */
void KateTabBar::updateFixedHeight()
{
    setFixedHeight(m_tabHeight);
    triggerResizeEvent();
}

void KateTabBar::updateSort()
{
    global_sortType = tabSortType();
    qSort(m_tabButtons.begin(), m_tabButtons.end(), tabLessThan);
    triggerResizeEvent();
}

/**
 * Triggers a resizeEvent. This is used whenever the tab buttons need
 * a rearrange. By using \p QApplication::sendEvent() multiple calls are
 * combined into only one call.
 *
 * \see addTab(), removeTab(), setMinimumWidth(), setMaximumWidth(),
 *      setFixedHeight()
 */
void KateTabBar::triggerResizeEvent()
{
    QResizeEvent ev(size(), size());
    QApplication::sendEvent(this, &ev);
}

