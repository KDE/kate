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

#ifndef KATE_TAB_BAR_H
#define KATE_TAB_BAR_H

#include <QWidget>

#include <QList>
#include <QMap>
#include <QIcon>
#include <QResizeEvent>

class KateTabButton;
class KConfigBase;

/**
 * The \p KateTabBar class provides a tab bar, e.g. for tabbed documents.
 *
 * The API closely follows the API of QTabBar.
 *
 * @author Dominik Haumann
 */
class KateTabBar : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool isActiveViewSpace READ isActiveViewSpace WRITE setActiveViewSpace)

public:
    KateTabBar(QWidget *parent = 0);
    virtual ~KateTabBar();

    /**
     * Adds a new tab with \a text. Returns the new tab's id.
     */
    int addTab(const QString &text);

    /**
     * Insert a tab at \p position with \a text. Returns the new tab's id.
     * @param position index of the tab, i.e. 0, ..., count()
     */
    int insertTab(int position, const QString & text);

    /**
     * Removes the tab with ID \a id.
     * @return the position where the tab was
     */
    int removeTab(int index);

    /**
     * Get the ID of the tab bar's activated tab. Returns -1 if no tab is activated.
     */
    int currentTab() const;

public Q_SLOTS:
    /**
     * Activate the tab with \p id. No signal is emitted.
     */
    void setCurrentTab(int index);   // does not emit signal

public:
    /**
     * Returns whether a tab with ID \a id exists.
     */
    bool containsTab(int index) const;

    /**
     * Set the button @p id's tool tip to @p tip.
     */
    void setTabToolTip(int index, const QString &tip);
    /**
     * Get the button @p id's url. Result is QStrint() if not available.
     */
    QString tabToolTip(int index) const;

    /**
     * Sets the text of the tab with ID \a id to \a text.
     * \see tabText()
     */
    void setTabText(int index, const QString &text);
    /**
     * Returns the text of the tab with ID \a id. If the button id does not
     * exist \a QString() is returned.
     * \see setTabText()
     */
    QString tabText(int index) const;

    /**
     * Sets the icon of the tab with ID \a id to \a icon.
     * \see tabIcon()
     */
    void setTabIcon(int index, const QIcon &pixmap);
    /**
     * Returns the icon of the tab with ID \a id. If the button id does not
     * exist \a QIcon() is returned.
     * \see setTabIcon()
     */
    QIcon tabIcon(int index) const;

    /**
     * Returns the number of tabs in the tab bar.
     */
    int count() const;

    void setHighlightMarks(const QMap<QString, QString> &marks);
    QMap<QString, QString> highlightMarks() const;

    /**
     * Return the maximum amount of tabs that fit into the tab bar given
     * the minimumTabWidth().
     */
    int maxTabCount() const;

    void setActiveViewSpace(bool active);
    bool isActiveViewSpace() const;

Q_SIGNALS:
    /**
     * This signal is emitted whenever the current activated tab changes.
     */
    void currentChanged(int id);

    /**
     * This signal is emitted whenever tab @p id should be closed.
     */
    void closeTabRequested(int id);

    /**
     * This signal is emitted whenever all tabs except tab @p id
     * should be closed.
     */
    void closeOtherTabsRequested(int id);

    /**
     * This signal is emitted whenever all tabs should be closed.
     */
    void closeAllTabsRequested();

    /**
     * This signal is emitted whenever a highlight mark changes.
     * Usually this is used to synchronice several tabbars.
     */
    void highlightMarksChanged(KateTabBar *tabbar);

    /**
     * This signal is emitted whenever the tab bar's width allows to
     * show more tabs than currently available. In other words,
     * you can safely add @p count tabs which are guaranteed to be visible.
     */
    void moreTabsRequested(int count);

    /**
     * This signal is emitted whenever the tab bar's width is too small,
     * such that not all tabs can be shown.
     * Therefore, @p count tabs should be removed.
     */
    void lessTabsRequested(int count);

    /**
     * This signal is emitted whenever the users double clicks on the free
     * space next to the tab bar. Typically, a new document should be
     * created.
     */
    void newTabRequested();

    /**
     * This signal is emitted whenever the tab bar was clicked while the
     * tab bar is not the active view space tab bar.
     */
    void activateViewSpaceRequested();

protected Q_SLOTS:
    /**
     * Active button changed. Emit signal \p currentChanged() with the button's ID.
     */
    void tabButtonActivated(KateTabButton *tabButton);

    /**
     * If the user wants to close a tab with the context menu, it sends a close
     * request.
     */
    void tabButtonCloseRequest(KateTabButton *tabButton);

protected:
    /**
     * Recalculate geometry for all tabs.
     */
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

    /**
     * Override to avoid requesting a new tab.
     */
    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    /**
     * Override to request making the tab bar active.
     */
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    /** trigger repaint on hover leave event */
    void leaveEvent(QEvent *event) Q_DECL_OVERRIDE;

protected:
    /**
     * Set tab geometry. The tabs are animated only if @p animate is @e true.
     */
    void updateButtonPositions(bool animate = false);

private:
    // minimum and maximum tab width
    int m_minimumTabWidth;
    int m_maximumTabWidth;

    // current tab width: when closing tabs with the mouse, we keep
    // the tab width fixed until the mouse leaves the tab bar. This
    // way the user can keep klicking the close button without moving
    // the ouse.
    qreal m_currentTabWidth;
    bool m_keepTabWidth;

    bool m_isActiveViewSpace;

    QList< KateTabButton * > m_tabButtons;
    QMap< int, KateTabButton * > m_idToTab;

    KateTabButton *m_activeButton;

    int m_nextID;
};

#endif // KATE_TAB_BAR_H
