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

#include <QVector>
#include <QHash>
#include <QIcon>

class KateTabButton;
class KateTabBarPrivate;

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
    Q_PROPERTY(bool isActive READ isActive WRITE setActive)

public:
    explicit KateTabBar(QWidget *parent = nullptr);
    ~KateTabBar() override;

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

    /**
     * Get the ID of the tab that is located left of the current tab.
     * The return value is -1, if there is no previous tab.
     */
    int prevTab() const;

    /**
     * Get the ID of the tab that is located right of the current tab.
     * The return value is -1, if there is no next tab.
     */
    int nextTab() const;

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
     * Sets the URL of the tab with ID \a id to \a url.
     * \see tabUrl()
     * \since 17.08
     */
    void setTabUrl(int index, const QUrl &url);

    /**
     * Returns the text of the tab with ID \a id. If the button id does not
     * exist \a QString() is returned.
     * \see setTabUrl()
     * \since 17.08
     */
    QUrl tabUrl(int index) const;

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

    /**
     * Return the maximum amount of tabs that fit into the tab bar given
     * the minimumTabWidth().
     */
    int maxTabCount() const;

    /**
     * Marks this tabbar as active. That is, current-tab indicators are
     * properly highlighted, indicating that child widgets of this tabbar
     * will get input.
     *
     * This concept is mostly useful, if your application has multiple tabbars.
     * Inactive tabbars are grayed out.
     */
    void setActive(bool active);

    /**
     * Returns whether this tabbar is active.
     */
    bool isActive() const;

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
     * This signal is emitted whenever the context menu is requested for
     * button @p id at position @p globalPos.
     * @param id the button, or -1 if the context menu was requested on
     *        at a place where no tab exists
     * @param globalPos the position of the context menu in global coordinates
     */
    void contextMenuRequest(int id, const QPoint & globalPos);

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
    //! Recalculate geometry for all tabs.
    void resizeEvent(QResizeEvent *event) override;

    //! Override to avoid requesting a new tab.
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    //! Override to request making the tab bar active.
    void mousePressEvent(QMouseEvent *event) override;

    //! trigger repaint on hover leave event
    void leaveEvent(QEvent *event) override;

    //! Paint tab separators
    void paintEvent(QPaintEvent *event) override;

    //! Request context menu
    void contextMenuEvent(QContextMenuEvent *ev) override;

    //! Cycle through tabs
    void wheelEvent(QWheelEvent * event) override;

    //! Support for drag & drop of tabs
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    // pimpl data holder
    KateTabBarPrivate * const d;
};

#endif // KATE_TAB_BAR_H
