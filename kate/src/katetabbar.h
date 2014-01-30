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
    Q_PROPERTY(bool isActiveViewSpace READ isActiveViewSpace WRITE setActiveViewSpace)

public:
    // NOTE: as the API here is very self-explaining the docs are in the cpp
    //       file, more clean imho.

    KateTabBar(QWidget *parent = 0);
    virtual ~KateTabBar();

    void load(KConfigBase *config, const QString &group);
    void save(KConfigBase *config, const QString &group) const;

    int addTab(const QString &text);
    int insertTab(int position, const QString & text);
    int removeTab(int index);

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

    void setHighlightMarks(const QMap<QString, QString> &marks);
    QMap<QString, QString> highlightMarks() const;

    int maxTabCount() const;

    void setActiveViewSpace(bool active);
    bool isActiveViewSpace() const;

public Q_SLOTS:
    void setCurrentTab(int index);   // does not emit signal
    void removeHighlightMarks();

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

protected Q_SLOTS:
    void tabButtonActivated(KateTabButton *tabButton);
    void tabButtonHighlightChanged(KateTabButton *tabButton);
    void tabButtonCloseRequest(KateTabButton *tabButton);

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

protected:
    void updateButtonPositions();

private:
    int m_minimumTabWidth;
    int m_maximumTabWidth;

    bool m_isActiveViewSpace;

    QList< KateTabButton * > m_tabButtons;
    QMap< int, KateTabButton * > m_idToTab;

    KateTabButton *m_activeButton;

    int m_nextID;

    // map of highlighted tabs and colors
    QMap< QString, QString > m_highlightedTabs;
};

#endif // KATE_TAB_BAR_H
