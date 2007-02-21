/***************************************************************************
                           ktinytabbarconfigdialog.cpp
                           -------------------
    begin                : 2005-06-19
    copyright            : (C) 2005 by Dominik Haumann
    email                : dhdev@gmx.de
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

#include "ktinytabbarconfigdialog.h"
#include "ktinytabbarconfigpage.h"
#include "ktinytabbar.h"
#include "ktinytabbutton.h"

#include <klocale.h>

#include <QSpinBox>

KTinyTabBarConfigDialog::KTinyTabBarConfigDialog( const KTinyTabBar* tabbar,
                                                  QWidget *parent )
    : KDialog( parent )
{
    setCaption( i18n( "Configure Tab Bar" ) );
    setButtons( KDialog::Cancel | KDialog::Ok );

    m_configPage = new KTinyTabBarConfigPage( this );

    m_configPage->setLocationTop( tabbar->locationTop() );
    m_configPage->setNumberOfRows( tabbar->numRows() );
    m_configPage->setMinimumTabWidth( tabbar->minimumTabWidth() );
    m_configPage->setMaximumTabWidth( tabbar->maximumTabWidth() );
    m_configPage->setFixedTabHeight( tabbar->tabHeight() );
    m_configPage->setFollowCurrentTab( tabbar->followCurrentTab() );
    m_configPage->setTabSortType( tabbar->tabSortType() );
    m_configPage->setTabButtonStyle( tabbar->tabButtonStyle() );
    m_configPage->setPlainColorActivated( tabbar->plainColorActivated() );
    m_configPage->setPlainColorHovered( tabbar->plainColorHovered() );
    m_configPage->setPlainColorPressed( tabbar->plainColorPressed() );
    m_configPage->setHighlightModifiedTabs( tabbar->highlightModifiedTabs() );
    m_configPage->setHighlightActiveTab( tabbar->highlightActiveTab() );
    m_configPage->setHighlightPreviousTab( tabbar->highlightPreviousTab() );
    m_configPage->setModifiedTabsColor( tabbar->modifiedTabsColor() );
    m_configPage->setActiveTabColor( tabbar->activeTabColor() );
    m_configPage->setPreviousTabColor( tabbar->previousTabColor() );
    m_configPage->setHighlightOpacity( tabbar->highlightOpacity() );

    setMainWidget( m_configPage );
    resize( 400, 300 );

    enableButton( KDialog::Ok, false );
    connect( m_configPage, SIGNAL( changed() ), this, SLOT( configChanged() ) );
    connect( m_configPage, SIGNAL( removeHighlightMarks() ),
             tabbar, SLOT( removeHighlightMarks() ) );
}

KTinyTabBarConfigDialog::~KTinyTabBarConfigDialog()
{
}

void KTinyTabBarConfigDialog::configChanged()
{
    enableButton( KDialog::Ok, true );
}

KTinyTabBarConfigPage* KTinyTabBarConfigDialog::configPage()
{
    return m_configPage;
}

#include "ktinytabbarconfigdialog.moc"

// kate: space-indent on; tab-width 4; replace-tabs off; eol unix;

