/***************************************************************************
                           ktinytabbar.cpp
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

#include "ktinytabbar.h"
#include "ktinytabbar.moc"
#include "ktinytabbutton.h"
#include "ktinytabbarconfigpage.h"
#include "ktinytabbarconfigdialog.h"

#include <kdebug.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kiconloader.h>

#include <QApplication> // QApplication::sendEvent
#include <QtAlgorithms> // qSort

KTinyTabBar::SortType global_sortType;

// public operator < for two tab buttons
bool tabLessThan( const KTinyTabButton* a, const KTinyTabButton* b )
{
    switch( global_sortType )
    {
        case KTinyTabBar::OpeningOrder: {
            return a->buttonID() < b->buttonID();
        }

        case KTinyTabBar::Name: {
            // fall back to ID
            if( a->text().toLower() == b->text().toLower() )
                return a->buttonID() < b->buttonID();

            return a->text().toLower() < b->text().toLower();
        }

        case KTinyTabBar::URL: {
            // fall back, if infos not available
            if( a->url().isEmpty() && b->url().isEmpty() )
            {
                if( a->text().toLower() == b->text().toLower() )
                    return a->buttonID() < b->buttonID();

                return a->text().toLower() < b->text().toLower();
            }

            return a->url().toLower() < b->url().toLower();
        }

        case KTinyTabBar::Extension:
        {
            // sort by extension, but check whether the files have an
            // extension first
            const int apos = a->text().lastIndexOf( '.' );
            const int bpos = b->text().lastIndexOf( '.' );

            if( apos == -1 && bpos == -1 )
                return a->text().toLower() < b->text().toLower();
            else if( apos == -1 )
                return true;
            else if( bpos == -1 )
                return false;
            else
            {
                const int aright = a->text().size() - apos;
                const int bright = b->text().size() - bpos;
                QString aExt = a->text().right( aright ).toLower();
                QString bExt = b->text().right( bright ).toLower();
                QString aFile = a->text().left( apos ).toLower();
                QString bFile = b->text().left( bpos ).toLower();

                if( aExt == bExt )
                    return (aFile == bFile) ? a->buttonID() < b->buttonID()
                                            : aFile < bFile;
                else
                    return aExt < bExt;
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
 *  - minimum tab width: 70 pixel
 *  - maximum tab width: 150 pixel
 *  - fixed tab height : 22 pixel. Note that the icon's size is 16 pixel.
 *  - number of rows: 1 row.
 *  .
 */
KTinyTabBar::KTinyTabBar( QWidget *parent )
    : QWidget( parent )
{
    m_minimumTabWidth = 70;
    m_maximumTabWidth = 150;

    m_tabHeight = 22;

    m_locationTop = false;
    m_numRows = 1;
    m_currentRow = 0;
    m_followCurrentTab = true;
    m_highlightModifiedTabs = false;
    m_highlightPreviousTab = false;
    m_highlightActiveTab = false;
    m_highlightOpacity = 20;

    m_tabButtonStyle = Push;
    m_sortType = OpeningOrder;
    m_nextID = 0;

    m_activeButton = 0L;
    m_previousButton = 0L;

    // default colors
    m_colorModifiedTab = QColor( Qt::red );
    m_colorActiveTab = QColor( 150, 150, 255 );
    m_colorPreviousTab = QColor( 150, 150, 255 );

    // functions called in ::load() will set settings for the nav buttons
    m_upButton = new KTinyTabButton( QString(), QString(), -1, true, this );
    m_downButton = new KTinyTabButton( QString(), QString(), -2, true, this );
    m_configureButton = new KTinyTabButton( QString(), QString(), -3, true, this );
    m_navigateSize = 20;

    m_upButton->setIcon( KIconLoader::global()->loadIcon( "arrow-up", KIconLoader::Small, 16 ) );
    m_downButton->setIcon( KIconLoader::global()->loadIcon( "arrow-down", KIconLoader::Small, 16 ) );
    m_configureButton->setIcon( KIconLoader::global()->loadIcon( "configure", KIconLoader::Small, 16 ) );

    connect( m_upButton, SIGNAL( activated( KTinyTabButton* ) ), this, SLOT( upClicked() ) );
    connect( m_downButton, SIGNAL( activated( KTinyTabButton* ) ), this, SLOT( downClicked() ) );
    connect( m_configureButton, SIGNAL( activated( KTinyTabButton* ) ), this, SLOT( configureClicked() ) );

    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    updateFixedHeight();
}

/**
 * Destroys the tab bar.
 */
KTinyTabBar::~KTinyTabBar()
{
}

/**
 * Loads the settings from \a config from section \a group.
 * Remembered properties are:
 *  - number of rows
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
void KTinyTabBar::load( KConfigBase* config, const QString& group )
{
    KConfigGroup cg( config, group );

    // tabbar properties
    setLocationTop    ( cg.readEntry( "location top", false ) );
    setNumRows        ( cg.readEntry( "count of rows", 1 ) );
    setMinimumTabWidth( cg.readEntry( "minimum width", 150 ) );
    setMaximumTabWidth( cg.readEntry( "maximum width", 300 ) );
    setTabHeight      ( cg.readEntry( "fixed height", 20 ) );
    setTabSortType    ( (SortType) cg.readEntry( "sort type", (int)OpeningOrder ) );
    setTabButtonStyle ( (ButtonStyle) cg.readEntry( "button style", (int)Push ) );
    setFollowCurrentTab(cg.readEntry("follow current tab", true ) );
    setHighlightModifiedTabs( cg.readEntry( "highlight modified", false ) );
    setHighlightPreviousTab( cg.readEntry( "highlight previous", false ) );
    setHighlightActiveTab( cg.readEntry( "highlight active", false ) );
    setHighlightOpacity(cg.readEntry( "highlight opacity", 20 ) );

    // color settings
    setModifiedTabsColor( cg.readEntry( "color modified", m_colorModifiedTab ) );
    setActiveTabColor( cg.readEntry( "color active", m_colorActiveTab ) );
    setPreviousTabColor( cg.readEntry( "color previous", m_colorPreviousTab ) );

    // highlighted entries
    QStringList documents = cg.readEntry( "highlighted documents", QStringList() );
    QStringList colors = cg.readEntry( "highlighted colors", QStringList() );

    // restore highlight map
    m_highlightedTabs.clear();
    for( int i = 0; i < documents.size() && i < colors.size(); ++i )
        m_highlightedTabs[documents[i]] = colors[i];
}

/**
 * Saves the settings to \a config into section \a group.
 * The original group is saved and restored at the end of this function.
 * See @p load() for more information.
 */
void KTinyTabBar::save( KConfigBase* config, const QString& group ) const
{
    KConfigGroup cg( config, group );

    // tabbar properties
    cg.writeEntry( "location top", locationTop() );
    cg.writeEntry( "count of rows", numRows() );
    cg.writeEntry( "minimum width", minimumTabWidth() );
    cg.writeEntry( "maximum width", maximumTabWidth() );
    cg.writeEntry( "fixed height", tabHeight() );
    cg.writeEntry( "sort type", (int)tabSortType() );
    cg.writeEntry( "button style", (int) tabButtonStyle() );
    cg.writeEntry( "follow current tab", followCurrentTab() );
    cg.writeEntry( "highlight modified", highlightModifiedTabs() );
    cg.writeEntry( "highlight previous", highlightPreviousTab() );
    cg.writeEntry( "highlight active", highlightActiveTab() );
    cg.writeEntry( "highlight opacity", highlightOpacity() );


    // color settings
    cg.writeEntry( "color modified", modifiedTabsColor() );
    cg.writeEntry( "color active", activeTabColor() );
    cg.writeEntry( "color previous", previousTabColor() );

    // highlighted entries
    cg.writeEntry( "highlighted documents", m_highlightedTabs.keys() );
    cg.writeEntry( "highlighted colors", m_highlightedTabs.values() );
}

/**
 * Set the location to @p top.
 */
void KTinyTabBar::setLocationTop( bool top )
{
    m_locationTop = top;
}

/**
 * Get whether the location is on top or bottom.
 */
bool KTinyTabBar::locationTop() const
{
    return m_locationTop;
}


/**
 * Set the number of \a rows.
 */
void KTinyTabBar::setNumRows( int rows )
{
    if( rows < 1 || rows == numRows() )
        return;

    m_numRows = rows;
    updateFixedHeight();
}

/**
 * Get the number of rows.
 */
int KTinyTabBar::numRows() const
{
    return m_numRows;
}

/**
 * Set the minimum width in pixels a tab must have.
 */
void KTinyTabBar::setMinimumTabWidth( int min_pixel )
{
    if( m_minimumTabWidth == min_pixel )
        return;

    m_minimumTabWidth = min_pixel;
    triggerResizeEvent();
}

/**
 * Set the maximum width in pixels a tab may have.
 */
void KTinyTabBar::setMaximumTabWidth( int max_pixel )
{
    if( m_maximumTabWidth == max_pixel )
        return;

    m_maximumTabWidth = max_pixel;
    triggerResizeEvent();
}

/**
 * Get the minimum width in pixels a tab can have.
 */
int KTinyTabBar::minimumTabWidth() const
{
    return m_minimumTabWidth;
}

/**
 * Get the maximum width in pixels a tab can have.
 */
int KTinyTabBar::maximumTabWidth() const
{
    return m_maximumTabWidth;
}

/**
 * Set the fixed height in pixels all tabs have.
 * \note If you also show icons use a height of iconheight + 2.
 *       E.g. for 16x16 pixel icons, a tab height of 18 pixel looks best.
 *       For 22x22 pixel icons a height of 24 pixel is best etc.
 */
void KTinyTabBar::setTabHeight( int height_pixel )
{
    if( m_tabHeight == height_pixel )
        return;

    m_tabHeight = height_pixel;
    updateFixedHeight();
}

/**
 * Get the fixed tab height in pixels.
 */
int KTinyTabBar::tabHeight() const
{
    return m_tabHeight;
}

/**
 * Adds a new tab with text \a text. Returns the new tab's ID. The document's
 * url @p docurl is used to sort the documents by URL.
 */
int KTinyTabBar::addTab( const QString& docurl, const QString& text )
{
    return addTab( docurl, QIcon(), text );
}

/**
 * This is an overloaded member function, provided for convenience.
 * It behaves essentially like the above function.
 *
 * Adds a new tab with \a icon and \a text. Returns the new tab's index.
 */
int KTinyTabBar::addTab( const QString& docurl, const QIcon& icon, const QString& text )
{
    KTinyTabButton* tabButton = new KTinyTabButton( docurl, text, m_nextID, false, this );
    tabButton->setIcon( icon );
    if( m_highlightedTabs.contains( text ) )
        tabButton->setHighlightColor( QColor( m_highlightedTabs[text] ) );

    // set all properties! be sure nothing is missing :)
    tabButton->setHighlightOpacity( highlightOpacity() );
    tabButton->setTabButtonStyle( tabButtonStyle() );
    tabButton->setHighlightModifiedTabs( highlightModifiedTabs() );
    tabButton->setHighlightActiveTab( highlightActiveTab() );
    tabButton->setHighlightPreviousTab( highlightPreviousTab() );
    tabButton->setModifiedTabsColor( modifiedTabsColor() );
    tabButton->setActiveTabColor( activeTabColor() );
    tabButton->setPreviousTabColor( previousTabColor() );

    m_tabButtons.append( tabButton );
    m_IDToTabButton[m_nextID] = tabButton;
    connect( tabButton, SIGNAL( activated( KTinyTabButton* ) ),
             this, SLOT( tabButtonActivated( KTinyTabButton* ) ) );
    connect( tabButton, SIGNAL( highlightChanged( KTinyTabButton* ) ),
             this, SLOT( tabButtonHighlightChanged( KTinyTabButton* ) ) );
    connect( tabButton, SIGNAL( closeRequest( KTinyTabButton* ) ),
             this, SLOT( tabButtonCloseRequest( KTinyTabButton* ) ) );
    connect( tabButton, SIGNAL( closeOtherTabsRequest( KTinyTabButton* ) ),
             this, SLOT( tabButtonCloseOtherRequest( KTinyTabButton* ) ) );
    connect( tabButton, SIGNAL( closeAllTabsRequest() ),
             this, SLOT( tabButtonCloseAllRequest() ) );

    if( !isVisible() )
        show();

    updateSort();

    return m_nextID++;
}

/**
 * Get the ID of the tab bar's activated tab. Returns -1 if no tab is activated.
 */
int KTinyTabBar::currentTab() const
{
    if( m_activeButton != 0L )
        return m_activeButton->buttonID();

    return -1;
}

/**
 * Activate the tab with ID \a button_id. No signal is emitted.
 */
void KTinyTabBar::setCurrentTab( int button_id )
{
    if( !m_IDToTabButton.contains( button_id ) )
        return;

    KTinyTabButton* tabButton = m_IDToTabButton[button_id];
    if( m_activeButton == tabButton )
        return;

    if( m_previousButton )
        m_previousButton->setPreviousTab( false );

    if( m_activeButton )
    {
        m_activeButton->setActivated( false );
        m_previousButton = m_activeButton;
        m_previousButton->setPreviousTab( true );
    }

    m_activeButton = tabButton;
    m_activeButton->setActivated( true );
    m_activeButton->setPreviousTab( false );

    // make current tab visible
    if( followCurrentTab() && !m_activeButton->isVisible() )
        makeCurrentTabVisible();
}

/**
 * Removes the tab with ID \a button_id.
 */
void KTinyTabBar::removeTab( int button_id )
{
    if( !m_IDToTabButton.contains( button_id ) )
        return;

    KTinyTabButton* tabButton = m_IDToTabButton[button_id];

    if( tabButton == m_previousButton )
        m_previousButton = 0L;

    if( tabButton == m_activeButton )
        m_activeButton = 0L;

    m_IDToTabButton.remove( button_id );
    m_tabButtons.removeAll( tabButton );
    // delete the button with deleteLater() because the button itself might
    // have send a close-request. So the app-execution is still in the
    // button, a delete tabButton; would lead to a crash.
    tabButton->hide();
    tabButton->deleteLater();

    if( m_tabButtons.count() == 0 )
        hide();

    triggerResizeEvent();
}

/**
 * Returns whether a tab with ID \a button_id exists.
 */
bool KTinyTabBar::containsTab( int button_id ) const
{
    return m_IDToTabButton.contains( button_id );
}

/**
 * Sets the text of the tab with ID \a button_id to \a text.
 * \see tabText()
 */
void KTinyTabBar::setTabText( int button_id, const QString& text )
{
    if( !m_IDToTabButton.contains( button_id ) )
        return;

    // change highlight key, if entry exists
    if( m_highlightedTabs.contains( m_IDToTabButton[button_id]->text() ) )
    {
        QString value = m_highlightedTabs[m_IDToTabButton[button_id]->text()];
        m_highlightedTabs.remove( m_IDToTabButton[button_id]->text() );
        m_highlightedTabs[text] = value;

        // do not emit highlightMarksChanged(), because every tabbar gets this
        // change (usually)
        // emit highlightMarksChanged( this );
    }

    m_IDToTabButton[button_id]->setText( text );

    if( tabSortType() == Name || tabSortType() == URL || tabSortType() == Extension )
        updateSort();
}

/**
 * Returns the text of the tab with ID \a button_id. If the button id does not
 * exist \a QString() is returned.
 * \see setTabText()
 */
QString KTinyTabBar::tabText( int button_id ) const
{
    if( m_IDToTabButton.contains( button_id ) )
        return m_IDToTabButton[button_id]->text();

    return QString();
}

/**
 * Set the button @p button_id's url to @p docurl.
 */
void KTinyTabBar::setTabURL( int button_id, const QString& docurl )
{
    if( !m_IDToTabButton.contains( button_id ) )
        return;

    m_IDToTabButton[button_id]->setURL( docurl);

    if( tabSortType() == URL )
        updateSort();
}

/**
 * Get the button @p button_id's url. Result is QStrint() if not available.
 */
QString KTinyTabBar::tabURL( int button_id ) const
{
    if( m_IDToTabButton.contains( button_id ) )
        return m_IDToTabButton[button_id]->url();

    return QString();
}


/**
 * Sets the icon of the tab with ID \a button_id to \a icon.
 * \see tabIcon()
 */
void KTinyTabBar::setTabIcon( int button_id, const QIcon& icon )
{
    if( m_IDToTabButton.contains( button_id ) )
        m_IDToTabButton[button_id]->setIcon( icon );
}

/**
 * Returns the icon of the tab with ID \a button_id. If the button id does not
 * exist \a QIcon() is returned.
 * \see setTabIcon()
 */
QIcon KTinyTabBar::tabIcon( int button_id ) const
{
    if( m_IDToTabButton.contains( button_id ) )
        return m_IDToTabButton[button_id]->icon();

    return QIcon();
}

/**
 * Returns the number of tabs in the tab bar.
 */
int KTinyTabBar::count() const
{
    return m_tabButtons.count();
}

/**
 * Set the tabbutton style. The signal @p tabButtonStyleChanged() is not
 * emitted.
 * @param tabStyle button style
 */
void KTinyTabBar::setTabButtonStyle( ButtonStyle tabStyle )
{
    m_tabButtonStyle = tabStyle;

    KTinyTabButton* tabButton;
    foreach( tabButton, m_tabButtons )
        tabButton->setTabButtonStyle( tabStyle );

    m_upButton->setTabButtonStyle( tabStyle );
    m_downButton->setTabButtonStyle( tabStyle );
    m_configureButton->setTabButtonStyle( tabStyle );
}

/**
 * Get the tabbutton style.
 * @return button style
 */
KTinyTabBar::ButtonStyle KTinyTabBar::tabButtonStyle() const
{
    return m_tabButtonStyle;
}

/**
 * Set the sort tye to @p sort.
 */
void KTinyTabBar::setTabSortType( SortType sort )
{
    if( m_sortType == sort )
        return;

    m_sortType = sort;
    updateSort();
}

/**
 * Get the sort type.
 */
KTinyTabBar::SortType KTinyTabBar::tabSortType() const
{
    return m_sortType;
}


/**
 * Set follow current tab to @p follow.
 */
void KTinyTabBar::setFollowCurrentTab( bool follow )
{
    m_followCurrentTab = follow;
    if( m_followCurrentTab )
        makeCurrentTabVisible();
}

/**
 * Check, whether to follow the current tab.
 */
bool KTinyTabBar::followCurrentTab() const
{
    return m_followCurrentTab;
}

/**
 * Set whether to highlight the previous button to @e highlight.
 */
void KTinyTabBar::setHighlightPreviousTab( bool highlight )
{
    m_highlightPreviousTab = highlight;

    KTinyTabButton* tabButton;
    foreach( tabButton, m_tabButtons )
        tabButton->setHighlightPreviousTab( highlight );
}

/**
 * Check, whether to highlight the previous button.
 */
bool KTinyTabBar::highlightPreviousTab() const
{
    return m_highlightPreviousTab;
}

/**
 * Set whether to highlight the previous button to @e highlight.
 */
void KTinyTabBar::setHighlightActiveTab( bool highlight )
{
    m_highlightActiveTab = highlight;

    KTinyTabButton* tabButton;
    foreach( tabButton, m_tabButtons )
        tabButton->setHighlightActiveTab( highlight );
}

/**
 * Check, whether to highlight the previous button.
 */
bool KTinyTabBar::highlightActiveTab() const
{
    return m_highlightActiveTab;
}

/**
 * Set the highlight opacity to @p value.
 */
void KTinyTabBar::setHighlightOpacity( int value )
{
    m_highlightOpacity = value;

    KTinyTabButton* tabButton;
    foreach( tabButton, m_tabButtons )
        tabButton->setHighlightOpacity( value );
}

/**
 * Get the highlight opacity.
 */
int KTinyTabBar::highlightOpacity() const
{
    return m_highlightOpacity;
}

/**
 * Set the color for the previous tab to @p color.
 */
void KTinyTabBar::setPreviousTabColor( const QColor& color )
{
    m_colorPreviousTab = color;
    KTinyTabButton* tabButton;
    foreach( tabButton, m_tabButtons )
        tabButton->setPreviousTabColor( color );
}

/**
 * Get the color for the previous tab.
 */
QColor KTinyTabBar::previousTabColor() const
{
    return m_colorPreviousTab;
}

/**
 * Set the color for the active tab to @p color.
 */
void KTinyTabBar::setActiveTabColor( const QColor& color )
{
    m_colorActiveTab = color;
    KTinyTabButton* tabButton;
    foreach( tabButton, m_tabButtons )
        tabButton->setActiveTabColor( color );
}

/**
 * Get the color for the active tab.
 */
QColor KTinyTabBar::activeTabColor() const
{
    return m_colorActiveTab;
}

void KTinyTabBar::setTabModified( int button_id, bool modified )
{
    if( m_IDToTabButton.contains( button_id ) )
        m_IDToTabButton[button_id]->setModified( modified);
}

bool KTinyTabBar::isTabModified( int button_id ) const
{
    if( m_IDToTabButton.contains( button_id ) )
        return m_IDToTabButton[button_id]->isModified();

    return false;
}

void KTinyTabBar::setHighlightModifiedTabs( bool modified )
{
    m_highlightModifiedTabs = modified;
    KTinyTabButton* tabButton;
    foreach( tabButton, m_tabButtons )
        tabButton->setHighlightModifiedTabs( modified );
}

bool KTinyTabBar::highlightModifiedTabs() const
{
    return m_highlightModifiedTabs;
}

void KTinyTabBar::setModifiedTabsColor( const QColor& color )
{
    m_colorModifiedTab = color;
    KTinyTabButton* tabButton;
    foreach( tabButton, m_tabButtons )
        tabButton->setModifiedTabsColor( color );
}

QColor KTinyTabBar::modifiedTabsColor() const
{
    return m_colorModifiedTab;
}

void KTinyTabBar::removeHighlightMarks()
{
    KTinyTabButton* tabButton;
    foreach( tabButton, m_tabButtons )
    {
        if( tabButton->highlightColor().isValid() )
            tabButton->setHighlightColor( QColor() );
    }

    m_highlightedTabs.clear();
    emit highlightMarksChanged( this );
}

void KTinyTabBar::setHighlightMarks( const QMap<QString, QString>& marks )
{
    m_highlightedTabs = marks;

    KTinyTabButton* tabButton;
    foreach( tabButton, m_tabButtons )
    {
        if( marks.contains( tabButton->text() ) )
        {
            if( tabButton->highlightColor().name() != marks[tabButton->text()] )
                tabButton->setHighlightColor( QColor( marks[tabButton->text()] ) );
        }
        else if( tabButton->highlightColor().isValid() )
            tabButton->setHighlightColor( QColor() );
    }
}

QMap<QString, QString> KTinyTabBar::highlightMarks() const
{
    return m_highlightedTabs;
}
//END public member functions






//BEGIN protected / private member functions

/**
 * Active button changed. Emit signal \p currentChanged() with the button's ID.
 */
void KTinyTabBar::tabButtonActivated( KTinyTabButton* tabButton )
{
    if( tabButton == m_activeButton )
        return;

    if( m_previousButton )
        m_previousButton->setPreviousTab( false );

    if( m_activeButton )
    {
        m_activeButton->setActivated( false );
        m_previousButton = m_activeButton;
        m_previousButton->setPreviousTab( true );
    }

    m_activeButton = tabButton;
    m_activeButton->setActivated( true );
    m_activeButton->setPreviousTab( false );

    emit currentChanged( tabButton->buttonID() );
}

/**
 * The \e tabButton's highlight color changed, so update the list of documents
 * and colors.
 */
void KTinyTabBar::tabButtonHighlightChanged( KTinyTabButton* tabButton )
{
    if( tabButton->highlightColor().isValid() )
    {
        m_highlightedTabs[tabButton->text()] = tabButton->highlightColor().name();
        emit highlightMarksChanged( this );
    }
    else if( m_highlightedTabs.contains( tabButton->text() ) )
    {
        // invalid color, so remove the item
        m_highlightedTabs.remove( tabButton->text() );
        emit highlightMarksChanged( this );
    }
}

/**
 * If the user wants to close a tab with the context menu, it sends a close
 * request. Throw the close request by emitting the signal @p closeRequest().
 */
void KTinyTabBar::tabButtonCloseRequest( KTinyTabButton* tabButton )
{
    emit closeRequest( tabButton->buttonID() );
}

/**
 * If the user wants to close all tabs except the current one using the context
 * menu, it sends multiple close requests.
 * Throw the close requests by emitting the signal @p closeRequest().
 */
void KTinyTabBar::tabButtonCloseOtherRequest( KTinyTabButton* tabButton )
{
    QList <int> tabToCloseID;
    for (int i = 0; i < m_tabButtons.size(); ++i) {
        if ((m_tabButtons.at(i))->buttonID() != tabButton->buttonID())
            tabToCloseID << (m_tabButtons.at(i))->buttonID();
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
void KTinyTabBar::tabButtonCloseAllRequest( )
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
void KTinyTabBar::resizeEvent( QResizeEvent* event )
{
//     kDebug(13040) << "resizeEvent";
    // if there are no tabs there is nothing to do. Do not delete otherwise
    // division by zero is possible.
    if( m_tabButtons.count() == 0 )
    {
        updateHelperButtons( event->size(), 0 );
        return;
    }

    int tabbar_width = event->size().width() - ( 4 - ( numRows()>3?3:numRows() ) ) * m_navigateSize;
    int tabs_per_row = tabbar_width / minimumTabWidth();
    if( tabs_per_row == 0 )
        tabs_per_row = 1;

    int tab_width = minimumTabWidth();

    int needed_rows = m_tabButtons.count() / tabs_per_row;
    if( needed_rows * tabs_per_row < (int)m_tabButtons.count() )
        ++needed_rows;

    // if we do not need more rows than available we can increase the tab
    // buttons' width up to maximumTabWidth.
    if( needed_rows <= numRows() )
    {
        // use available size optimal, but honor maximumTabWidth()
        tab_width = tabbar_width * numRows() / m_tabButtons.count();

        if( tab_width > maximumTabWidth() )
            tab_width = maximumTabWidth();

        tabs_per_row = tabbar_width / tab_width;

        // due to rounding fuzzys we have to increase the tabs_per_row if
        // the number of tabs does not fit.
        if( tabs_per_row * numRows() < (int)m_tabButtons.count() )
            ++tabs_per_row;
    }

    // On this point, we really know the value of tabs_per_row. So a final
    // calculation gives us the tab_width. With this the width can even get
    // greater than maximumTabWidth(), but that does not matter as it looks
    // more ugly if there is a lot wasted space on the right.
    tab_width = tabbar_width / tabs_per_row;

    updateHelperButtons( event->size(), needed_rows );

    KTinyTabButton* tabButton;

    foreach( tabButton, m_tabButtons )
        tabButton->hide();

    for( int row = 0; row < numRows(); ++row )
    {
        int current_row = row + currentRow();
        for( int i = 0; i < tabs_per_row; ++i )
        {
            // value returns 0L, if index is out of bounds
            tabButton = m_tabButtons.value( current_row * tabs_per_row + i );

            if( tabButton )
            {
                tabButton->setGeometry( i * tab_width, row * tabHeight(),
                                        tab_width, tabHeight() );
                tabButton->show();
            }
        }
    }
}

/**
 * Make sure the current tab visible. If it is not visible the tabbar scrolls
 * so that it is visible.
 */
void KTinyTabBar::makeCurrentTabVisible()
{
    if( !m_activeButton || m_activeButton->isVisible() )
        return;

    //BEGIN copy of resizeEvent
    int tabbar_width = width() - ( 4 - ( numRows()>3?3:numRows() ) ) * m_navigateSize;
    int tabs_per_row = tabbar_width / minimumTabWidth();
    if( tabs_per_row == 0 )
        tabs_per_row = 1;

    int tab_width = minimumTabWidth();

    int needed_rows = m_tabButtons.count() / tabs_per_row;
    if( needed_rows * tabs_per_row < (int)m_tabButtons.count() )
        ++needed_rows;

    // if we do not need more rows than available we can increase the tab
    // buttons' width up to maximumTabWidth.
    if( needed_rows <= numRows() )
    {
        // use available size optimal, but honor maximumTabWidth()
        tab_width = tabbar_width * numRows() / m_tabButtons.count();

        if( tab_width > maximumTabWidth() )
            tab_width = maximumTabWidth();

        tabs_per_row = tabbar_width / tab_width;

        // due to rounding fuzzys we have to increase the tabs_per_row if
        // the number of tabs does not fit.
        if( tabs_per_row * numRows() < (int)m_tabButtons.count() )
            ++tabs_per_row;
    }
    //END copy of resizeEvent

    int index = m_tabButtons.indexOf( m_activeButton );
    int firstVisible = currentRow() * tabs_per_row;
    int lastVisible = ( currentRow() + numRows() ) * tabs_per_row - 1;

    if( firstVisible >= m_tabButtons.count() )
        firstVisible = m_tabButtons.count() - 1;

    if( lastVisible >= m_tabButtons.count() )
        lastVisible = m_tabButtons.count() - 1;

    if( index < firstVisible )
    {
        setCurrentRow( index / tabs_per_row );
    }
    else if( index > lastVisible )
    {
        const int diff = index / tabs_per_row - ( numRows() - 1 );
        setCurrentRow( diff );
    }
}

/**
 * Updates the fixed height. Called when the tab height or the number of rows
 * changed.
 */
void KTinyTabBar::updateFixedHeight()
{
    setFixedHeight( numRows() * tabHeight() );
    triggerResizeEvent();
}

/**
 * May modifies current row if more tabs fit into a row.
 * Sets geometry for the buttons 'up', 'down' and 'configure'.
 */
void KTinyTabBar::updateHelperButtons( QSize new_size, int needed_rows )
{
    // if the size increased so that more tabs fit into one row it can happen
    // that suddenly a row on the bottom is empty - or that even all rows are
    // empty. That is not desired, so make sure that currentRow has a
    // reasonable value.
    if( currentRow() + numRows() > needed_rows )
        m_currentRow = ( needed_rows - numRows() < 0 ?
                         0 : needed_rows - numRows() );

    m_upButton->setEnabled( currentRow() != 0 );
    m_downButton->setEnabled( needed_rows - currentRow() > numRows() );

    // set geometry for up, down, configure
    switch( numRows() )
    {
    case 1:
        m_upButton->setGeometry( new_size.width() - 3 * m_navigateSize,
                                 0, m_navigateSize, tabHeight() );
        m_downButton->setGeometry( new_size.width() - 2 * m_navigateSize,
                                   0, m_navigateSize, tabHeight() );
        m_configureButton->setGeometry( new_size.width() - m_navigateSize,
                                        0, m_navigateSize, tabHeight() );
        break;
    case 2:
        m_upButton->setGeometry( new_size.width() - 2 * m_navigateSize,
                                 0, m_navigateSize, tabHeight() );
        m_downButton->setGeometry( new_size.width() - 2 * m_navigateSize,
                                   tabHeight(), m_navigateSize, tabHeight() );
        m_configureButton->setGeometry( new_size.width() - m_navigateSize,
                                        0, m_navigateSize, 2 * tabHeight() );
        break;
    default:
        m_upButton->setGeometry( new_size.width() - m_navigateSize,
                                 0, m_navigateSize, tabHeight() );
        m_downButton->setGeometry( new_size.width() - m_navigateSize,
                                   tabHeight(), m_navigateSize, tabHeight() );
        m_configureButton->setGeometry( new_size.width() - m_navigateSize,
                                        2 * tabHeight(), m_navigateSize, tabHeight() );
        break;
    }
}

void KTinyTabBar::updateSort()
{
    global_sortType = tabSortType();
    qSort( m_tabButtons.begin(), m_tabButtons.end(), tabLessThan );
    triggerResizeEvent();
}

/**
 * Decrease the current row. Called when the button 'up' was clicked.
 */
void KTinyTabBar::upClicked()
{
    decreaseCurrentRow();
    m_upButton->setActivated( false );
}

/**
 * Increase the current row. Called when the button 'down' was clicked.
 */
void KTinyTabBar::downClicked()
{
    increaseCurrentRow();
    m_downButton->setActivated( false );
}

/**
 * Show configure dialog. If the button style changes the signal
 * @p tabButtonStyleChanged() will be emitted.
 */
void KTinyTabBar::configureClicked()
{
    m_configureButton->setActivated( false );

    KTinyTabBarConfigDialog dlg( this, (QWidget*)parent() );
    dlg.setObjectName( "tabbar_config_dialog" );
    if( dlg.exec() == KDialog::Accepted )
    {
        KTinyTabBarConfigPage* page = dlg.configPage();

        setLocationTop( page->locationTop() );
        setNumRows( page->numberOfRows() );
        setMinimumTabWidth( page->minimumTabWidth() );
        setMaximumTabWidth( page->maximumTabWidth() );
        setTabHeight( page->fixedTabHeight() );
        setTabSortType( page->tabSortType() );
        setTabButtonStyle( page->tabButtonStyle() );
        setFollowCurrentTab( page->followCurrentTab() );
        setHighlightModifiedTabs( page->highlightModifiedTabs() );
        setHighlightActiveTab( page->highlightActiveTab() );
        setHighlightPreviousTab( page->highlightPreviousTab() );
        setModifiedTabsColor( page->modifiedTabsColor() );
        setActiveTabColor( page->activeTabColor() );
        setPreviousTabColor( page->previousTabColor() );
        setHighlightOpacity( page->highlightOpacity() );

        emit settingsChanged( this );
    }
}

/**
 * Set the current row.
 * @param row new current row
 */
void KTinyTabBar::setCurrentRow( int row )
{
    if( row == currentRow() )
        return;

    m_currentRow = row;
    if( m_currentRow < 0 )
        m_currentRow = 0;

    triggerResizeEvent();
}

/**
 * Returns the current row.
 */
int KTinyTabBar::currentRow() const
{
    return m_currentRow;
}

/**
 * Increase the current row.
 */
void KTinyTabBar::increaseCurrentRow()
{
    ++m_currentRow;
    triggerResizeEvent();
}

/**
 * Decrease the current row.
 */
void KTinyTabBar::decreaseCurrentRow()
{
    if( m_currentRow == 0 )
        return;

    --m_currentRow;
    triggerResizeEvent();
}

/**
 * Triggers a resizeEvent. This is used whenever the tab buttons need
 * a rearrange. By using \p QApplication::sendEvent() multiple calls are
 * combined into only one call.
 *
 * \see addTab(), removeTab(), setMinimumWidth(), setMaximumWidth(),
 *      setFixedHeight(), increaseCurrentRow(), decreaseCurrentRow()
 */
void KTinyTabBar::triggerResizeEvent()
{
    QResizeEvent ev( size(), size() );
    QApplication::sendEvent( this, &ev );
}

//END protected / private member functions

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs off; eol unix;
