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

    case KateTabBar::URL: {
        // fall back, if infos not available
        if (a->url().isEmpty() && b->url().isEmpty()) {
            if (a->text().toLower() == b->text().toLower()) {
                return a->buttonID() < b->buttonID();
            }

            return a->text() < b->text();
//                 return KStringHandler::naturalCompare(a->text(), b->text(), Qt::CaseInsensitive) < 0; FIXME KF5
        }

        return a->url() < b->url();
        //             return KStringHandler::naturalCompare(a->url(), b->url(), Qt::CaseInsensitive) < 0; FIXME KF5
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

//BEGIN public member functions
/**
 * Creates a new tab bar with the given \a parent and \a name.
 *
 * The default values are in detail:
 *  - minimum tab width: 150 pixel
 *  - maximum tab width: 200 pixel
 *  - fixed tab height : 22 pixel. Note that the icon's size is 16 pixel.
 *  .
 */
KateTabBar::KateTabBar(QWidget *parent)
    : QWidget(parent)
{
    m_minimumTabWidth = 150;
    m_maximumTabWidth = 200;

    m_tabHeight = 22;

    m_sortType = OpeningOrder;
    m_nextID = 0;

    m_activeButton = 0L;

    // functions called in ::load() will set settings for the nav buttons
    m_moreButton = new QToolButton(this);
    m_moreButton->setAutoRaise(true);
    m_moreButton->setText(i18n("..."));
    m_moreButton->setToolTip(i18n("Quick Open"));
    connect(m_moreButton, SIGNAL(clicked()), this, SIGNAL(moreButtonClicked()));

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
    setMinimumTabWidth(cg.readEntry("minimum width", 150));
    setMaximumTabWidth(cg.readEntry("maximum width", 300));
    setTabHeight(cg.readEntry("fixed height", 20));
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
    cg.writeEntry("fixed height", tabHeight());
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
 * Set the fixed height in pixels all tabs have.
 * \note If you also show icons use a height of iconheight + 2.
 *       E.g. for 16x16 pixel icons, a tab height of 18 pixel looks best.
 *       For 22x22 pixel icons a height of 24 pixel is best etc.
 */
void KateTabBar::setTabHeight(int height_pixel)
{
    if (m_tabHeight == height_pixel) {
        return;
    }

    m_tabHeight = height_pixel;
    updateFixedHeight();
}

/**
 * Get the fixed tab height in pixels.
 */
int KateTabBar::tabHeight() const
{
    return m_tabHeight;
}

/**
 * Adds a new tab with text \a text. Returns the new tab's ID. The document's
 * url @p docurl is used to sort the documents by URL.
 */
int KateTabBar::addTab(const QString &docurl, const QString &text)
{
    return addTab(docurl, QIcon(), text);
}

/**
 * This is an overloaded member function, provided for convenience.
 * It behaves essentially like the above function.
 *
 * Adds a new tab with \a icon and \a text. Returns the new tab's index.
 */
int KateTabBar::addTab(const QString &docurl, const QIcon &icon, const QString &text)
{
    KateTabButton *tabButton = new KateTabButton(docurl, text, m_nextID, this);
    tabButton->setIcon(icon);
    if (m_highlightedTabs.contains(text)) {
        tabButton->setHighlightColor(QColor(m_highlightedTabs[text]));
    }

    m_tabButtons.append(tabButton);
    m_IDToTabButton[m_nextID] = tabButton;
    connect(tabButton, SIGNAL(activated(KateTabButton *)),
            this, SLOT(tabButtonActivated(KateTabButton *)));
    connect(tabButton, SIGNAL(highlightChanged(KateTabButton *)),
            this, SLOT(tabButtonHighlightChanged(KateTabButton *)));
    connect(tabButton, SIGNAL(closeRequest(KateTabButton *)),
            this, SLOT(tabButtonCloseRequest(KateTabButton *)));
    connect(tabButton, SIGNAL(closeOtherTabsRequest(KateTabButton *)),
            this, SLOT(tabButtonCloseOtherRequest(KateTabButton *)));
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
    int index = m_tabButtons.indexOf(button);
    Q_ASSERT(index > -1);

    m_tabButtons.move(index, 0);

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
 * Activate the tab with ID \a button_id. No signal is emitted.
 */
void KateTabBar::setCurrentTab(int button_id)
{
    if (!m_IDToTabButton.contains(button_id)) {
        return;
    }

    KateTabButton *tabButton = m_IDToTabButton[button_id];
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
 * Removes the tab with ID \a button_id.
 */
void KateTabBar::removeTab(int button_id)
{
    if (!m_IDToTabButton.contains(button_id)) {
        return;
    }

    KateTabButton *tabButton = m_IDToTabButton[button_id];

    if (tabButton == m_activeButton) {
        m_activeButton = 0L;
    }

    m_IDToTabButton.remove(button_id);
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
 * Returns whether a tab with ID \a button_id exists.
 */
bool KateTabBar::containsTab(int button_id) const
{
    return m_IDToTabButton.contains(button_id);
}

/**
 * Sets the text of the tab with ID \a button_id to \a text.
 * \see tabText()
 */
void KateTabBar::setTabText(int button_id, const QString &text)
{
    if (!m_IDToTabButton.contains(button_id)) {
        return;
    }

    // change highlight key, if entry exists
    if (m_highlightedTabs.contains(m_IDToTabButton[button_id]->text())) {
        QString value = m_highlightedTabs[m_IDToTabButton[button_id]->text()];
        m_highlightedTabs.remove(m_IDToTabButton[button_id]->text());
        m_highlightedTabs[text] = value;

        // do not emit highlightMarksChanged(), because every tabbar gets this
        // change (usually)
        // emit highlightMarksChanged( this );
    }

    m_IDToTabButton[button_id]->setText(text);

    if (tabSortType() == Name || tabSortType() == URL || tabSortType() == Extension) {
        updateSort();
    }
}

/**
 * Returns the text of the tab with ID \a button_id. If the button id does not
 * exist \a QString() is returned.
 * \see setTabText()
 */
QString KateTabBar::tabText(int button_id) const
{
    if (m_IDToTabButton.contains(button_id)) {
        return m_IDToTabButton[button_id]->text();
    }

    return QString();
}

/**
 * Set the button @p button_id's url to @p docurl.
 */
void KateTabBar::setTabURL(int button_id, const QString &docurl)
{
    if (!m_IDToTabButton.contains(button_id)) {
        return;
    }

    m_IDToTabButton[button_id]->setURL(docurl);

    if (tabSortType() == URL) {
        updateSort();
    }
}

/**
 * Get the button @p button_id's url. Result is QStrint() if not available.
 */
QString KateTabBar::tabURL(int button_id) const
{
    if (m_IDToTabButton.contains(button_id)) {
        return m_IDToTabButton[button_id]->url();
    }

    return QString();
}


/**
 * Sets the icon of the tab with ID \a button_id to \a icon.
 * \see tabIcon()
 */
void KateTabBar::setTabIcon(int button_id, const QIcon &icon)
{
    if (m_IDToTabButton.contains(button_id)) {
        m_IDToTabButton[button_id]->setIcon(icon);
    }
}

/**
 * Returns the icon of the tab with ID \a button_id. If the button id does not
 * exist \a QIcon() is returned.
 * \see setTabIcon()
 */
QIcon KateTabBar::tabIcon(int button_id) const
{
    if (m_IDToTabButton.contains(button_id)) {
        return m_IDToTabButton[button_id]->icon();
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

void KateTabBar::setTabModified(int button_id, bool modified)
{
    if (m_IDToTabButton.contains(button_id)) {
        m_IDToTabButton[button_id]->setModified(modified);
    }
}

bool KateTabBar::isTabModified(int button_id) const
{
    if (m_IDToTabButton.contains(button_id)) {
        return m_IDToTabButton[button_id]->isModified();
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
//END public member functions






//BEGIN protected / private member functions

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
    emit closeRequest(tabButton->buttonID());
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
        emit closeRequest(tabToCloseID.at(i));
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
        emit closeRequest(tabToCloseID.at(i));
    }
}

/**
 * Recalculate geometry for all children.
 */
void KateTabBar::resizeEvent(QResizeEvent *event)
{
//     qDebug() << "resizeEvent";
    // if there are no tabs there is nothing to do. Do not delete otherwise
    // division by zero is possible.
    if (m_tabButtons.count() == 0) {
        updateHelperButtons(event->size());
        return;
    }

    int tabbar_width = event->size().width() - m_navigateSize;
    int tabs_per_row = tabbar_width / minimumTabWidth();
    if (tabs_per_row == 0) {
        tabs_per_row = 1;
    }

    int tab_width = minimumTabWidth();

    // On this point, we really know the value of tabs_per_row. So a final
    // calculation gives us the tab_width. With this the width can even get
    // greater than maximumTabWidth(), but that does not matter as it looks
    // more ugly if there is a lot wasted space on the right.
    tab_width = tabbar_width / tabs_per_row;

    updateHelperButtons(event->size());

    KateTabButton *tabButton;

    foreach(tabButton, m_tabButtons)
    tabButton->hide();

    qDebug() << "---> tab width, items per row:" << tab_width << tabs_per_row;

    for (int i = 0; i < 10; ++i) {
        // value returns 0L, if index is out of bounds
        tabButton = m_tabButtons.value(i);

        if (tabButton) {
            tabButton->setGeometry(i * tab_width, 0,
                                   tab_width, tabHeight());
            tabButton->show();
        }
    }
}

/**
 * Updates the fixed height. Called when the tab height or the number of rows
 * changed.
 */
void KateTabBar::updateFixedHeight()
{
    setFixedHeight(tabHeight());
    triggerResizeEvent();
}

/**
 * May modifies current row if more tabs fit into a row.
 * Sets geometry for the buttons 'up', 'down' and 'configure'.
 */
void KateTabBar::updateHelperButtons(QSize new_size)
{
    m_moreButton->setGeometry(new_size.width() - m_moreButton->minimumSizeHint().width(),
                              0, m_moreButton->minimumSizeHint().width(), tabHeight());
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

//END protected / private member functions

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on; eol unix;
