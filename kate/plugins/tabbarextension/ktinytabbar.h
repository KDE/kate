/***************************************************************************
                           ktinytabbar.h
                           -------------------
    begin                : 2005-06-15
    copyright            : (C) 2005 by Dominik Haumann
    email                : dhdev@gmx.de
    
    Copyright (C) 2007 Flavio Castelli <flavio.castelli@gmail.com>
 ***************************************************************************/

/***************************************************************************
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ***************************************************************************/

#ifndef KTINYTABBAR_H
#define KTINYTABBAR_H

#include <QWidget>

#include <QList>
#include <QMap>
#include <QIcon>
#include <QResizeEvent>

class KTinyTabButton;
class KConfig;

/**
 * The \p KTinyTabBar class provides a tab bar, e.g. for tabbed documents and
 * supports multiple rows. The tab bar hides itself if there are no tabs.
 *
 * It implements the API from TrollTech's \p QTabBar with some minor changes
 * and additions.
 *
 * @author Dominik Haumann
 */
class KTinyTabBar : public QWidget
{
    Q_OBJECT

public:
    /**
     * Defines the tab button's style.
     */
    enum ButtonStyle
    {
        Push=0,   ///< default push button
        Flat,     ///< flat push button
        Plain     ///< plain push button (QFrame like)
    };

    /**
     * Sort types.
     */
    enum SortType
    {
        OpeningOrder=0, ///< opening order
        Name,           ///< alphabetically
        URL,            ///< alphabetically URL based
        Extension       ///< by file extension (suffix)
    };
    Q_DECLARE_FLAGS( SortTypes, SortType )

public:
    // NOTE: as the API here is very self-explaining the docs are in the cpp
    //       file, more clean imho.

    KTinyTabBar( QWidget *parent = 0 );
    virtual ~KTinyTabBar();

    void load( KConfig* config, const QString& group );
    void save( KConfig* config, const QString& group ) const;

    void setLocationTop( bool top );
    bool locationTop() const;

    void setNumRows( int rows );
    int numRows() const;

    void setMinimumTabWidth( int min_pixel );
    void setMaximumTabWidth( int max_pixel );

    int minimumTabWidth() const;
    int maximumTabWidth() const;

    void setTabHeight( int height_pixel );
    int tabHeight() const;

    int addTab( const QString& docurl, const QString& text );
    int addTab( const QString& docurl, const QIcon& pixmap, const QString& text );
    void removeTab( int button_id );

    int currentTab() const;
    // corresponding SLOT: void setCurrentTab( int button_id );

    bool containsTab( int button_id ) const;

    void setTabURL( int button_id, const QString& docurl );
    QString tabURL( int button_id ) const;

    void setTabText( int button_id, const QString& text );
    QString tabText( int button_id ) const;

    void setTabIcon( int button_id, const QIcon& pixmap );
    QIcon tabIcon( int button_id ) const;

    void setTabModified( int button_id, bool modified );
    bool isTabModified( int button_id ) const;

    void setHighlightModifiedTabs( bool modified );
    bool highlightModifiedTabs() const;

    void setModifiedTabsColor( const QColor& color );
    QColor modifiedTabsColor() const;

    int count() const;

    void setTabButtonStyle( ButtonStyle tabStyle );
    ButtonStyle tabButtonStyle() const;

    void setTabSortType( SortType sort );
    SortType tabSortType() const;

    void setFollowCurrentTab( bool follow );
    bool followCurrentTab() const;

    void setHighlightPreviousTab( bool highlight );
    bool highlightPreviousTab() const;

    void setHighlightActiveTab( bool highlight );
    bool highlightActiveTab() const;

    void setHighlightOpacity( int value );
    int highlightOpacity() const;

    void setPlainColorPressed( const QColor& color );
    QColor plainColorPressed() const;

    void setPlainColorHovered( const QColor& color );
    QColor plainColorHovered() const;

    void setPlainColorActivated( const QColor& color );
    QColor plainColorActivated() const;

    void setPreviousTabColor( const QColor& color );
    QColor previousTabColor() const;

    void setActiveTabColor( const QColor& color );
    QColor activeTabColor() const;

    void setHighlightMarks( const QMap<QString, QString>& marks );
    QMap<QString, QString> highlightMarks() const;

public slots:
    void setCurrentTab( int button_id ); // does not emit signal
    void removeHighlightMarks();

signals:
    /**
     * This signal is emitted whenever the current activated tab changes.
     */
    void currentChanged( int button_id );
    /**
     * This signal is emitted whenever a tab should be closed.
     */
    void closeRequest( int button_id );
    /**
     * This signal is emitted whenever a setting entry changes.
     * A special property is the location. As the tabbar is embedded it can not
     * change its location itself. So if you find a changed location, then go
     * and change it yourself!
     */
    void settingsChanged( KTinyTabBar* tabbar );

    /**
     * This signal is emitted whenever a highlight mark changes.
     * Usually this is used to synchronice several tabbars.
     */
    void highlightMarksChanged( KTinyTabBar* tabbar );

protected slots:
    void tabButtonActivated( KTinyTabButton* tabButton );
    void tabButtonHighlightChanged( KTinyTabButton* tabButton );
    void tabButtonCloseAllRequest();
    void tabButtonCloseRequest( KTinyTabButton* tabButton );
    void tabButtonCloseOtherRequest( KTinyTabButton* tabButton );
    void upClicked();
    void downClicked();
    void configureClicked();
    void makeCurrentTabVisible();

protected:
    virtual void resizeEvent( QResizeEvent* event );

protected:
    void updateFixedHeight();
    void triggerResizeEvent();
    void updateSort();
    int currentRow() const;
    void setCurrentRow( int row );
    void updateHelperButtons( QSize new_size, int needed_rows );
    void increaseCurrentRow();
    void decreaseCurrentRow();

private:
    bool m_locationTop;
    int m_numRows;
    int m_currentRow;
    int m_minimumTabWidth;
    int m_maximumTabWidth;
    int m_tabHeight;

    QList< KTinyTabButton* > m_tabButtons;
    QMap< int, KTinyTabButton* > m_IDToTabButton;

    KTinyTabButton* m_activeButton;
    KTinyTabButton* m_previousButton;

    // buttons on the right to navigate and configure
    KTinyTabButton* m_upButton;
    KTinyTabButton* m_downButton;
    KTinyTabButton* m_configureButton;
    int m_navigateSize;

    int m_nextID;

    // map of highlighted tabs and colors
    QMap< QString, QString > m_highlightedTabs;
    // tab button style
    ButtonStyle m_tabButtonStyle;
    SortType m_sortType;
    bool m_highlightModifiedTabs;
    bool m_followCurrentTab;
    bool m_highlightPreviousTab;
    bool m_highlightActiveTab;
    int m_highlightOpacity;

    // configurable and saved by KTinyTabBar
    QColor m_plainColorPressed;
    QColor m_plainColorHovered;
    QColor m_plainColorActivated;
    QColor m_colorModifiedTab;
    QColor m_colorActiveTab;
    QColor m_colorPreviousTab;
};

#endif // KTINYTABBAR_H

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs off; eol unix;
