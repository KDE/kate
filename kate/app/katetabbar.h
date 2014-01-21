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

/**
 * The \p KateTabBar class provides a tab bar, e.g. for tabbed documents and
 * supports multiple rows. The tab bar hides itself if there are no tabs.
 *
 * It implements the API from TrollTech's \p QTabBar with some minor changes
 * and additions.
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
        URL,            ///< alphabetically URL based
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

    int addTab(const QString &docurl, const QString &text);
    int addTab(const QString &docurl, const QIcon &pixmap, const QString &text);
    void removeTab(int button_id);

    int currentTab() const;
    // corresponding SLOT: void setCurrentTab( int button_id );

    bool containsTab(int button_id) const;

    void setTabURL(int button_id, const QString &docurl);
    QString tabURL(int button_id) const;

    void setTabText(int button_id, const QString &text);
    QString tabText(int button_id) const;

    void setTabIcon(int button_id, const QIcon &pixmap);
    QIcon tabIcon(int button_id) const;

    void setTabModified(int button_id, bool modified);
    bool isTabModified(int button_id) const;

    int count() const;

    void setTabSortType(SortType sort);
    SortType tabSortType() const;

    void setHighlightMarks(const QMap<QString, QString> &marks);
    QMap<QString, QString> highlightMarks() const;

public Q_SLOTS:
    void setCurrentTab(int button_id);   // does not emit signal
    void removeHighlightMarks();
    void raiseTab(int buttonId);

Q_SIGNALS:
    /**
     * This signal is emitted whenever the current activated tab changes.
     */
    void currentChanged(int button_id);
    /**
     * This signal is emitted whenever a tab should be closed.
     */
    void closeRequest(int button_id);
    /**
     * This signal is emitted whenever a setting entry changes.
     */
    void settingsChanged(KateTabBar *tabbar);

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
    void updateHelperButtons(QSize new_size);

private:
    int m_minimumTabWidth;
    int m_maximumTabWidth;
    int m_tabHeight;

    QList< KateTabButton * > m_tabButtons;
    QMap< int, KateTabButton * > m_IDToTabButton;

    KateTabButton *m_activeButton;

    // buttons on the right to navigate and configure
    KateTabButton *m_configureButton;
    int m_navigateSize;

    int m_nextID;

    // map of highlighted tabs and colors
    QMap< QString, QString > m_highlightedTabs;
    SortType m_sortType;
};

#endif // KATE_TAB_BAR_H

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on; eol unix;
