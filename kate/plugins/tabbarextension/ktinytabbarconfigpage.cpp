/***************************************************************************
                           ktinytabbarconfigpage.cpp
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

#include "ktinytabbarconfigpage.h"
#include "ktinytabbutton.h"

#include <QSpinBox>
#include <QGroupBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QSlider>

#include <kdialog.h>
#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kcolorbutton.h>
#include <ktabwidget.h>

#include <kdebug.h>

KTinyTabBarConfigPage::KTinyTabBarConfigPage( QWidget *parent )
    : QWidget( parent )
    , Ui::TabBarConfigWidget()
{
    setupUi(this);

    // preview group box
    QHBoxLayout* hlPreview = new QHBoxLayout( gbPreview );
    m_previewMinimum = new KTinyTabButton( QString(), i18n( "minimum size" ), 0, true, gbPreview );
    m_previewMaximum = new KTinyTabButton( QString(), i18n( "maximum size" ), 1, true, gbPreview );
    hlPreview->addWidget( m_previewMinimum );
    hlPreview->addWidget( m_previewMaximum );

    connect(btnClearCache, SIGNAL(clicked()),
            this, SIGNAL(removeHighlightMarks()));

    setupConnections();
}

void KTinyTabBarConfigPage::setupDefaults()
{
    // location
    cmbLocation->setCurrentIndex(0);

    // follow current tab
    chkFollowActive->setChecked(true);

    // sort by
    cmbSorting->setCurrentIndex(0);

    // tab sizes group box
    sbMinWidth->setValue(70);
    sbMaxWidth->setValue(150);
    sbHeight->setValue(22);

    // button style group
    cmbStyle->setCurrentIndex(0);

    // tab highlighting
    chkModified->setChecked(false);
    colModified->setEnabled(false);
    colModified->setColor(Qt::red);
    chkActive->setChecked(false);
    colActive->setEnabled(false);
    colActive->setColor(Qt::blue);
    chkPrevious->setChecked(false);
    colPrevious->setEnabled(false);
    colPrevious->setColor(Qt::yellow);

    slOpacity->setValue( 20 );


    // preview
    m_previewMinimum->setActivated( true );
    m_previewMinimum->setFixedSize( minimumTabWidth(), fixedTabHeight() );
    m_previewMaximum->setFixedSize( maximumTabWidth(), fixedTabHeight() );
    m_previewMinimum->setHighlightOpacity( highlightOpacity() );
    m_previewMaximum->setHighlightOpacity( highlightOpacity() );
    m_previewMinimum->setTabButtonStyle( tabButtonStyle() );
    m_previewMaximum->setTabButtonStyle( tabButtonStyle() );
    m_previewMinimum->setHighlightActiveTab( highlightActiveTab() );
    m_previewMinimum->setHighlightPreviousTab( highlightPreviousTab() );
    m_previewMaximum->setHighlightActiveTab( highlightActiveTab() );
    m_previewMaximum->setHighlightPreviousTab( highlightPreviousTab() );

    m_previewMinimum->setHighlightModifiedTabs( false );
    m_previewMaximum->setHighlightModifiedTabs( true );
    m_previewMaximum->setModifiedTabsColor( modifiedTabsColor() );
    m_previewMaximum->setModified( true );
}

void KTinyTabBarConfigPage::setupConnections()
{
    // location: nothing
    connect(cmbLocation, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));

    // rows group box
    connect(sbRows, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
    connect(chkFollowActive, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    // sort by group box
    connect(cmbSorting, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));

    // tab sizes group box
    connect(sbMinWidth, SIGNAL(valueChanged(int)), this, SLOT(minimumTabWidthChanged(int)));
    connect(sbMaxWidth, SIGNAL(valueChanged(int)), this, SLOT(maximumTabWidthChanged(int)));
    connect(sbHeight, SIGNAL(valueChanged(int)), this, SLOT(fixedTabHeightChanged(int)));

    // button style group
    connect(cmbStyle, SIGNAL(currentIndexChanged(int)), this, SLOT(buttonStyleChanged(int)));

    // tab highlighting
    connect(chkModified, SIGNAL(toggled(bool)), this, SLOT(highlightModifiedTabsChanged(bool)));
    connect(colModified, SIGNAL(changed(const QColor&)), this, SLOT(modifiedTabsColorChanged(const QColor&)));
    connect(chkActive, SIGNAL(toggled(bool)), this, SLOT(highlightActiveTabChanged(bool)));
    connect(chkPrevious, SIGNAL(toggled(bool)), this, SLOT(highlightPreviousTabChanged(bool)));
    connect(colActive, SIGNAL(changed(const QColor&)), this, SLOT(activeTabColorChanged(const QColor&)));
    connect(colPrevious, SIGNAL(changed(const QColor&)), this, SLOT(previousTabColorChanged(const QColor&)));
    connect(slOpacity, SIGNAL(sliderMoved(int)), this, SLOT(highlightOpacityChanged(int)));

    // preview
    connect(m_previewMinimum, SIGNAL(activated(KTinyTabButton*)), this, SLOT(buttonActivated(KTinyTabButton*)));
    connect(m_previewMaximum, SIGNAL(activated(KTinyTabButton*)), this, SLOT(buttonActivated(KTinyTabButton*)));
}

KTinyTabBarConfigPage::~KTinyTabBarConfigPage()
{
}

//BEGIN protected slots

void KTinyTabBarConfigPage::minimumTabWidthChanged( int value )
{
    m_previewMinimum->setFixedWidth( value );
    emit changed();
}

void KTinyTabBarConfigPage::maximumTabWidthChanged( int value )
{
    m_previewMaximum->setFixedWidth( value );
    emit changed();
}

void KTinyTabBarConfigPage::fixedTabHeightChanged( int value )
{
    m_previewMinimum->setFixedHeight( value );
    m_previewMaximum->setFixedHeight( value );
    emit changed();
}

void KTinyTabBarConfigPage::buttonStyleChanged(int index)
{
    KTinyTabBar::ButtonStyle style = static_cast<KTinyTabBar::ButtonStyle>(index);
    m_previewMinimum->setTabButtonStyle(style);
    m_previewMaximum->setTabButtonStyle(style);
    emit changed();
}

void KTinyTabBarConfigPage::highlightActiveTabChanged( bool highlight )
{
    m_previewMinimum->setHighlightActiveTab( highlight );
    m_previewMaximum->setHighlightActiveTab( highlight );
    emit changed();
}

void KTinyTabBarConfigPage::highlightPreviousTabChanged( bool highlight )
{
    m_previewMinimum->setHighlightPreviousTab( highlight );
    m_previewMaximum->setHighlightPreviousTab( highlight );
    emit changed();
}

void KTinyTabBarConfigPage::activeTabColorChanged( const QColor& newColor )
{
    m_previewMinimum->setActiveTabColor( newColor );
    m_previewMaximum->setActiveTabColor( newColor );
    emit changed();
}

void KTinyTabBarConfigPage::previousTabColorChanged( const QColor& newColor )
{
    m_previewMinimum->setPreviousTabColor( newColor );
    m_previewMaximum->setPreviousTabColor( newColor );
    emit changed();
}

void KTinyTabBarConfigPage::highlightOpacityChanged( int value )
{
    m_previewMinimum->setHighlightOpacity( value );
    m_previewMaximum->setHighlightOpacity( value );
    emit changed();
}

void KTinyTabBarConfigPage::highlightModifiedTabsChanged( bool highlight )
{
    m_previewMinimum->setHighlightModifiedTabs( highlight );
    m_previewMaximum->setHighlightModifiedTabs( highlight );
    emit changed();
}

void KTinyTabBarConfigPage::modifiedTabsColorChanged( const QColor& newColor )
{
    m_previewMinimum->setModifiedTabsColor( newColor );
    m_previewMaximum->setModifiedTabsColor( newColor );
    emit changed();
}




void KTinyTabBarConfigPage::buttonActivated( KTinyTabButton* button )
{
    if( button == m_previewMinimum ) {
        m_previewMinimum->setPreviousTab(false);
        m_previewMaximum->setActivated(false);
        m_previewMaximum->setPreviousTab(true);
    } else {
        m_previewMaximum->setPreviousTab(false);
        m_previewMinimum->setActivated(false);
        m_previewMinimum->setPreviousTab(true);
    }
}
//END protected slots

bool KTinyTabBarConfigPage::locationTop() const
{
    return cmbLocation->currentIndex() == 0;
}

void KTinyTabBarConfigPage::setLocationTop( bool value )
{
    cmbLocation->setCurrentIndex( value ? 0 : 1);
}

int KTinyTabBarConfigPage::minimumTabWidth() const
{
    return sbMinWidth->value();
}

void KTinyTabBarConfigPage::setMinimumTabWidth( int value )
{
    sbMinWidth->setValue( value );
}

int KTinyTabBarConfigPage::maximumTabWidth() const
{
    return sbMaxWidth->value();
}

void KTinyTabBarConfigPage::setMaximumTabWidth( int value )
{
    sbMaxWidth->setValue( value );
}

int KTinyTabBarConfigPage::fixedTabHeight() const
{
    return sbHeight->value();
}

void KTinyTabBarConfigPage::setFixedTabHeight( int value )
{
    sbHeight->setValue( value );
}


int KTinyTabBarConfigPage::numberOfRows() const
{
    return sbRows->value();
}

void KTinyTabBarConfigPage::setNumberOfRows( int value )
{
    sbRows->setValue( value );
}

bool KTinyTabBarConfigPage::followCurrentTab() const
{
    return chkFollowActive->isChecked();
}

void KTinyTabBarConfigPage::setFollowCurrentTab( bool value )
{
    chkFollowActive->setChecked( value );
}

void KTinyTabBarConfigPage::setTabSortType( KTinyTabBar::SortType type )
{
    int index = static_cast<int>(type);
    cmbSorting->setCurrentIndex(index);
}

KTinyTabBar::SortType KTinyTabBarConfigPage::tabSortType() const
{
    return static_cast<KTinyTabBar::SortType>(cmbSorting->currentIndex());
}


void KTinyTabBarConfigPage::setTabButtonStyle( KTinyTabBar::ButtonStyle style )
{
    int index = static_cast<int>(style);
    cmbStyle->setCurrentIndex(index);
}

KTinyTabBar::ButtonStyle KTinyTabBarConfigPage::tabButtonStyle() const
{
    return static_cast<KTinyTabBar::ButtonStyle>(cmbStyle->currentIndex());
}


void KTinyTabBarConfigPage::setHighlightActiveTab( bool value )
{
    chkActive->setChecked( value );
    m_previewMinimum->setHighlightActiveTab( value );
    m_previewMaximum->setHighlightActiveTab( value );
}

bool KTinyTabBarConfigPage::highlightActiveTab() const
{
    return chkActive->isChecked();
}

void KTinyTabBarConfigPage::setActiveTabColor( const QColor& color )
{
    colActive->setColor( color );
    m_previewMinimum->setActiveTabColor( color );
    m_previewMaximum->setActiveTabColor( color );
}

QColor KTinyTabBarConfigPage::activeTabColor() const
{
    return colActive->color();
}


void KTinyTabBarConfigPage::setHighlightPreviousTab( bool value )
{
    chkPrevious->setChecked( value );
    m_previewMinimum->setHighlightPreviousTab( value );
    m_previewMaximum->setHighlightPreviousTab( value );
}

bool KTinyTabBarConfigPage::highlightPreviousTab() const
{
    return chkPrevious->isChecked();
}

void KTinyTabBarConfigPage::setPreviousTabColor( const QColor& color )
{
    colPrevious->setColor( color );
    m_previewMinimum->setPreviousTabColor( color );
    m_previewMaximum->setPreviousTabColor( color );
}

QColor KTinyTabBarConfigPage::previousTabColor() const
{
    return colPrevious->color();
}

void KTinyTabBarConfigPage::setHighlightOpacity( int value )
{
    slOpacity->setValue( value );
    m_previewMinimum->setHighlightOpacity( value );
    m_previewMaximum->setHighlightOpacity( value );
}

int KTinyTabBarConfigPage::highlightOpacity() const
{
    return slOpacity->value();
}

void KTinyTabBarConfigPage::setHighlightModifiedTabs( bool modified )
{
    chkModified->setChecked( modified );
    m_previewMinimum->setHighlightModifiedTabs( modified );
    m_previewMaximum->setHighlightModifiedTabs( modified );
}

bool KTinyTabBarConfigPage::highlightModifiedTabs() const
{
    return chkModified->isChecked();
}

void KTinyTabBarConfigPage::setModifiedTabsColor( const QColor& color )
{
    colModified->setColor( color );
    m_previewMinimum->setModifiedTabsColor( color );
    m_previewMaximum->setModifiedTabsColor( color );
}

QColor KTinyTabBarConfigPage::modifiedTabsColor() const
{
    return colModified->color();
}



#include "ktinytabbarconfigpage.moc"

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs off; eol unix;
