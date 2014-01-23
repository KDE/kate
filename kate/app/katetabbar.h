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

#ifndef KATE_TAB_BAR_H
#define KATE_TAB_BAR_H

#include <QWidget>

#include <QList>
#include <QMap>
#include <QIcon>
#include <QResizeEvent>

class KateTabButton;
class KConfigBase;
class QToolButton;

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

public:
    /**
     * Sort types.
     */
    enum SortType {
        OpeningOrder = 0, ///< opening order
        Name,           ///< alphabetically
        Extension       ///< by file extension (suffix)
    };
    Q_DECLARE_FLAGS(SortTypes, SortType)

public:
    // NOTE: as the API here is very self-explaining the docs are in the cpp
    //       file, more clean imho.

    KateTabBar(QWidget *parent = 0);
    virtual ~KateTabBar();

    void load(KConfigBase *config, const QString &group);
    void save(KConfigBase *config, const QString &group) const;

    void setMinimumTabWidth(int min_pixel);
    void setMaximumTabWidth(int max_pixel);

    int minimumTabWidth() const;
    int maximumTabWidth() const;

    void setTabHeight(int height_pixel);
    int tabHeight() const;

    int addTab(const QString &text);
    int addTab(const QIcon &pixmap, const QString &text);
    void removeTab(int index);

    int currentTab() const;
    // corresponding SLOT: void setCurrentTab( int index );

    bool containsTab(int index) const;

    void setTabToolTip(int index, const QString &tip);
    QString tabToolTip(int index) const;

    void setTabText(int index, const QString &text);
    QString tabText(int index) const;

    void setTabIcon(int index, const QIcon &pixmap);
    QIcon tabIcon(int index) const;

    void setTabModified(int index, bool modified);
    bool isTabModified(int index) const;

    int count() const;

    void setTabSortType(SortType sort);
    SortType tabSortType() const;

    void setHighlightMarks(const QMap<QString, QString> &marks);
    QMap<QString, QString> highlightMarks() const;

    int maxTabCount() const;

public Q_SLOTS:
    void setCurrentTab(int index);   // does not emit signal
    void removeHighlightMarks();
    void raiseTab(int index);

Q_SIGNALS:
    /**
     * This signal is emitted whenever the current activated tab changes.
     */
    void currentChanged(int index);

    /**
     * This signal is emitted whenever a tab should be closed.
     */
    void tabCloseRequested(int index);

    /**
     * This signal is emitted whenever a highlight mark changes.
     * Usually this is used to synchronice several tabbars.
     */
    void highlightMarksChanged(KateTabBar *tabbar);

protected Q_SLOTS:
    void tabButtonActivated(KateTabButton *tabButton);
    void tabButtonHighlightChanged(KateTabButton *tabButton);
    void tabButtonCloseAllRequest();
    void tabButtonCloseRequest(KateTabButton *tabButton);
    void tabButtonCloseOtherRequest(KateTabButton *tabButton);

protected:
    virtual void resizeEvent(QResizeEvent *event);

protected:
    void updateFixedHeight();
    void triggerResizeEvent();
    void updateSort();

private:
    int m_minimumTabWidth;
    int m_maximumTabWidth;
    int m_tabHeight;

    QList< KateTabButton * > m_tabButtons;
    QMap< int, KateTabButton * > m_IDToTabButton;

    KateTabButton *m_activeButton;

    int m_nextID;

    // map of highlighted tabs and colors
    QMap< QString, QString > m_highlightedTabs;
    SortType m_sortType;
};

#endif // KATE_TAB_BAR_H

