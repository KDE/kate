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
#include <QTabWidget>
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

#include <kdebug.h>

KTinyTabBarConfigPage::KTinyTabBarConfigPage( QWidget *parent )
    : QWidget( parent )
{
    initialize( this );
}

void KTinyTabBarConfigPage::initialize( QWidget* parent )
{
    setupGUI( parent );
    setupDefaults();
    setupConnections();
}

void KTinyTabBarConfigPage::setupGUI( QWidget* parent )
{
    QVBoxLayout* top = new QVBoxLayout( parent );
    top->setSpacing( KDialog::spacingHint() );
    top->setMargin( 0 );
    QTabWidget* tabWidget = new QTabWidget( parent );
    top->addWidget( tabWidget );
    QGroupBox* gbPreview = new QGroupBox( i18n( "Preview" ), this );
    top->addWidget( gbPreview );

    QWidget* tabAppearance = new QWidget( tabWidget );
    QWidget* tabStyle = new QWidget( tabWidget );
    tabWidget->addTab( tabAppearance, i18n( "&Appearance" ) );
    tabWidget->addTab( tabStyle, i18n( "&Style" ) );

    // Appearance tab
    QGridLayout* glAppearance = new QGridLayout( tabAppearance, 2, 2, KDialog::marginHint(), KDialog::spacingHint() );
    QGroupBox* gbLocation = new QGroupBox( i18n( "Location" ), tabAppearance );
    QGroupBox* gbRows = new QGroupBox( i18n( "Rows" ), tabAppearance );
    QGroupBox* gbSortBy = new QGroupBox( i18n( "Sort By" ), tabAppearance );
    QGroupBox* gbTabSizes = new QGroupBox( i18n( "Tab Sizes" ), tabAppearance );

    glAppearance->addWidget( gbLocation, 0, 0 );
    glAppearance->addWidget( gbRows, 0, 1 );
    glAppearance->addWidget( gbSortBy, 1, 0 );
    glAppearance->addWidget( gbTabSizes, 1, 1 );

    // location group box
    QVBoxLayout* vlLocation = new QVBoxLayout( gbLocation );
    vlLocation->setMargin( KDialog::marginHint() );
    vlLocation->setSpacing( KDialog::spacingHint() );
    m_locationTop = new QRadioButton( i18n( "&Top" ), gbLocation );
    m_locationBottom = new QRadioButton( i18n( "&Bottom" ), gbLocation );
    vlLocation->addWidget( m_locationTop );
    vlLocation->addWidget( m_locationBottom );

    // rows group box
    QGridLayout* glRows = new QGridLayout( gbRows, 2, 2, KDialog::marginHint(), KDialog::spacingHint() );
    QLabel* lblNumberOfRows = new QLabel( i18n( "Number of &rows (1-10):" ), gbRows );
    m_rowsRows = new QSpinBox( gbRows );
    m_rowsRows->setSuffix( i18n( "row(s)" ) );
    m_rowsFollowCurrentTab = new QCheckBox( i18n( "&follow current tab" ), gbRows );
    lblNumberOfRows->setBuddy( m_rowsRows );
    glRows->addWidget( lblNumberOfRows, 0, 0 );
    glRows->addWidget( m_rowsRows, 0, 1 );
    glRows->addWidget( m_rowsFollowCurrentTab, 1, 0, 1, 2 );

    // sort by group box
    QVBoxLayout* vlSortBy = new QVBoxLayout( gbSortBy );
    vlSortBy->setMargin( KDialog::marginHint() );
    vlSortBy->setSpacing( KDialog::spacingHint() );
    m_sortByOpening = new QRadioButton( i18n( "&opening order" ), gbSortBy );
    m_sortByName = new QRadioButton( i18n( "document &name" ), gbSortBy );
    m_sortByURL = new QRadioButton( i18n( "document &URL" ), gbSortBy );
    m_sortByExtension = new QRadioButton( i18n( "file e&xtension" ), gbSortBy );
    vlSortBy->addWidget( m_sortByOpening );
    vlSortBy->addWidget( m_sortByName );
    vlSortBy->addWidget( m_sortByURL );
    vlSortBy->addWidget( m_sortByExtension );

    // tab sizes group box
    QGridLayout* glTabSizes = new QGridLayout( gbTabSizes, 3, 2, KDialog::marginHint(), KDialog::spacingHint() );
    QLabel* lblMinimumWidth = new QLabel( i18n( "&Minimum tab width:" ), gbTabSizes );
    QLabel* lblMaximumWidth = new QLabel( i18n( "M&aximum tab width:" ), gbTabSizes );
    QLabel* lblFixedHeight = new QLabel( i18n( "F&ixed tab height:" ), gbTabSizes );
    m_tabSizesMinimumWidth = new QSpinBox( gbTabSizes );
    m_tabSizesMaximumWidth = new QSpinBox( gbTabSizes );
    m_tabSizesFixedHeight = new QSpinBox( gbTabSizes );
    m_tabSizesMinimumWidth->setSuffix( i18n( "pixels" ) );
    m_tabSizesMaximumWidth->setSuffix( i18n( "pixels" ) );
    m_tabSizesFixedHeight->setSuffix( i18n( "pixels" ) );
    lblMinimumWidth->setBuddy( m_tabSizesMinimumWidth );
    lblMaximumWidth->setBuddy( m_tabSizesMaximumWidth );
    lblFixedHeight->setBuddy( m_tabSizesFixedHeight );
    glTabSizes->addWidget( lblMinimumWidth, 0, 0 );
    glTabSizes->addWidget( lblMaximumWidth, 1, 0 );
    glTabSizes->addWidget( lblFixedHeight, 2, 0 );
    glTabSizes->addWidget( m_tabSizesMinimumWidth, 0, 1 );
    glTabSizes->addWidget( m_tabSizesMaximumWidth, 1, 1 );
    glTabSizes->addWidget( m_tabSizesFixedHeight, 2, 1 );

    // Style tab
    QGridLayout* glStyle = new QGridLayout(  tabStyle, 2, 2, KDialog::marginHint(), KDialog::spacingHint() );
    QGroupBox* gbButtonStyle = new QGroupBox( i18n( "Button Style" ), tabStyle );
    QGroupBox* gbPlainButtonColors = new QGroupBox( i18n( "Frame Colors" ), tabStyle );
    QGroupBox* gbHighlighting= new QGroupBox( i18n( "Tab Highlighting" ), tabStyle );

    glStyle->addWidget( gbButtonStyle, 0, 0 );
    glStyle->addWidget( gbPlainButtonColors, 0, 1 );
    glStyle->addWidget( gbHighlighting, 1, 0, 1, 2 );

    // button style group box
    QVBoxLayout* vlButtonStyle = new QVBoxLayout( gbButtonStyle );
    vlButtonStyle->setMargin( KDialog::marginHint() );
    vlButtonStyle->setSpacing( KDialog::spacingHint() );
    m_buttonStylePush = new QRadioButton( i18n( "&default push button" ), gbButtonStyle );
    m_buttonStyleFlat = new QRadioButton( i18n( "f&lat push button" ), gbButtonStyle );
    m_buttonStylePlain = new QRadioButton( i18n( "&plain frames" ), gbButtonStyle );
    vlButtonStyle->addWidget( m_buttonStylePush );
    vlButtonStyle->addWidget( m_buttonStyleFlat );
    vlButtonStyle->addWidget( m_buttonStylePlain );
    gbPlainButtonColors->setEnabled( false );
    connect( m_buttonStylePlain, SIGNAL( toggled( bool ) ), gbPlainButtonColors, SLOT( setEnabled( bool ) ) );

    // plain button colors group box
    QGridLayout* glPlainButtonColors = new QGridLayout( gbPlainButtonColors, 3, 2, KDialog::marginHint(), KDialog::spacingHint() );
    QLabel* lblActivated = new QLabel( i18n( "Acti&vated button" ), gbPlainButtonColors );
    QLabel* lblHovered = new QLabel( i18n( "&Hovered button" ), gbPlainButtonColors );
    QLabel* lblPressed = new QLabel( i18n( "P&ressed button" ), gbPlainButtonColors );
    m_plainButtonColorsActivated = new KColorButton( gbPlainButtonColors );
    m_plainButtonColorsHovered = new KColorButton( gbPlainButtonColors );
    m_plainButtonColorsPressed = new KColorButton( gbPlainButtonColors );
    lblActivated->setBuddy( m_plainButtonColorsActivated );
    lblHovered->setBuddy( m_plainButtonColorsHovered );
    lblPressed->setBuddy( m_plainButtonColorsPressed );
    glPlainButtonColors->addWidget( lblActivated, 0, 0 );
    glPlainButtonColors->addWidget( lblHovered, 1, 0 );
    glPlainButtonColors->addWidget( lblPressed, 2, 0 );
    glPlainButtonColors->addWidget( m_plainButtonColorsActivated, 0, 1 );
    glPlainButtonColors->addWidget( m_plainButtonColorsHovered, 1, 1 );
    glPlainButtonColors->addWidget( m_plainButtonColorsPressed, 2, 1 );

    // tab button highlighting group box
    QGridLayout* glHighlighting = new QGridLayout( gbHighlighting, 6, 2, KDialog::marginHint(), KDialog::spacingHint() );
    m_highlightingModifiedTabs = new QCheckBox( i18n( "highlight &modified tabs" ), gbHighlighting );
    m_highlightingModifiedTabsColor = new KColorButton( gbHighlighting );
    m_highlightingActiveTab = new QCheckBox( i18n( "highlight a&ctive tab" ), gbHighlighting );
    m_highlightingActiveTabColor = new KColorButton( gbHighlighting );
    m_highlightingPreviousTab = new QCheckBox( i18n( "highlight pr&evious tab" ), gbHighlighting );
    m_highlightingPreviousTabColor = new KColorButton( gbHighlighting );
    m_highlightingOpacityLabel = new QLabel( i18n( "&Opacity of highlighted button (100%):" ), gbHighlighting);
    m_highlightingOpacity = new QSlider( Qt::Horizontal, gbHighlighting );
    QPushButton* removeHighlighting = new QPushButton( i18n( "Remove all highlight mar&ks" ), gbHighlighting );
    removeHighlighting->setToolTip( i18n( "Remove all highlight marks in the current session." ) );
    QLabel* lblNote = new QLabel( i18n( "Note: Use the context menu to highlight a tab." ), gbHighlighting );
    m_highlightingOpacityLabel->setBuddy( m_highlightingOpacity );
    glHighlighting->addWidget( m_highlightingModifiedTabs, 0, 0 );
    glHighlighting->addWidget( m_highlightingModifiedTabsColor, 0, 1 );
    glHighlighting->addWidget( m_highlightingActiveTab, 1, 0 );
    glHighlighting->addWidget( m_highlightingActiveTabColor, 1, 1 );
    glHighlighting->addWidget( m_highlightingPreviousTab, 2, 0 );
    glHighlighting->addWidget( m_highlightingPreviousTabColor, 2, 1 );
    glHighlighting->addWidget( m_highlightingOpacityLabel, 3, 0 );
    glHighlighting->addWidget( m_highlightingOpacity, 3, 1 );
    glHighlighting->addWidget( removeHighlighting, 4, 0 );
    glHighlighting->addWidget( lblNote, 5, 0, 1, 2 );
    connect( m_highlightingModifiedTabs, SIGNAL( toggled( bool ) ),
             m_highlightingModifiedTabsColor, SLOT( setEnabled( bool ) ) );
    connect( m_highlightingActiveTab, SIGNAL( toggled( bool ) ),
             m_highlightingActiveTabColor, SLOT( setEnabled( bool ) ) );
    connect( m_highlightingPreviousTab, SIGNAL( toggled( bool ) ),
             m_highlightingPreviousTabColor, SLOT( setEnabled( bool ) ) );
    connect( removeHighlighting, SIGNAL( clicked() ),
             this, SIGNAL( removeHighlightMarks() ) );

    // preview group box
    QHBoxLayout* hlPreview = new QHBoxLayout( gbPreview );
    m_previewMinimum = new KTinyTabButton( QString(), i18n( "minimum size" ), 0, true, gbPreview );
    m_previewMaximum = new KTinyTabButton( QString(), i18n( "maximum size" ), 1, true, gbPreview );
    hlPreview->addWidget( m_previewMinimum );
    hlPreview->addWidget( m_previewMaximum );
}

void KTinyTabBarConfigPage::setupDefaults()
{
    // location
    m_locationBottom->setChecked( true );

    // rows group box
    m_rowsRows->setRange( 1, 10 );
    m_rowsFollowCurrentTab->setChecked( true );

    // sort by
    m_sortByOpening->setChecked( true );

    // tab sizes group box
    m_tabSizesMinimumWidth->setRange( 20, 500 );
    m_tabSizesMaximumWidth->setRange( 20, 500 );
    m_tabSizesFixedHeight->setRange( 16, 50 );
    m_tabSizesMinimumWidth->setValue( 70 );
    m_tabSizesMaximumWidth->setValue( 150 );
    m_tabSizesFixedHeight->setValue( 18 );

    // button style group
    m_buttonStylePush->setChecked( true );

    // plain button colors
    m_plainButtonColorsActivated->setColor( QColor( 128, 128, 255 ) );
    m_plainButtonColorsHovered->setColor( QColor( 255, 128, 128 ) );
    m_plainButtonColorsPressed->setColor( QColor( 128, 128, 255 ) );


    // tab highlighting
    m_highlightingModifiedTabs->setChecked( false );
    m_highlightingModifiedTabsColor->setEnabled( false );
    m_highlightingModifiedTabsColor->setColor( Qt::red );
    m_highlightingActiveTab->setChecked( false );
    m_highlightingActiveTabColor->setColor( Qt::blue );
    m_highlightingActiveTabColor->setEnabled( false );
    m_highlightingPreviousTab->setChecked( false );
    m_highlightingPreviousTabColor->setColor( Qt::yellow );
    m_highlightingPreviousTabColor->setEnabled( false );
    m_highlightingOpacity->setRange( 0, 100 );
    m_highlightingOpacity->setValue( 20 );


    // preview
    m_previewMinimum->setActivated( true );
    m_previewMinimum->setFixedSize( minimumTabWidth(), fixedTabHeight() );
    m_previewMaximum->setFixedSize( maximumTabWidth(), fixedTabHeight() );
    m_previewMinimum->setHighlightOpacity( highlightOpacity() );
    m_previewMaximum->setHighlightOpacity( highlightOpacity() );
    m_previewMinimum->setTabButtonStyle( tabButtonStyle() );
    m_previewMaximum->setTabButtonStyle( tabButtonStyle() );
    m_previewMinimum->setPlainColorActivated( plainColorActivated() );
    m_previewMinimum->setPlainColorPressed( plainColorPressed() );
    m_previewMinimum->setPlainColorHovered( plainColorHovered() );
    m_previewMaximum->setPlainColorActivated( plainColorActivated() );
    m_previewMaximum->setPlainColorPressed( plainColorPressed() );
    m_previewMaximum->setPlainColorHovered( plainColorHovered() );
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
    connect( m_locationTop, SIGNAL( toggled( bool ) ),
             this, SLOT (locationTopChanged( bool ) ) );

    // rows group box
    connect( m_rowsRows, SIGNAL( valueChanged( int ) ),
             this, SLOT( numberOfRowsChanged( int ) ) );
    connect( m_rowsFollowCurrentTab, SIGNAL( toggled( bool ) ),
             this, SLOT( followCurrentTabChanged( bool ) ) );

    // sort by group box
    connect( m_sortByOpening, SIGNAL( toggled( bool ) ),
             this, SLOT( sortTypeChanged( bool ) ) );
    connect( m_sortByName, SIGNAL( toggled( bool ) ),
             this, SLOT( sortTypeChanged( bool ) ) );
    connect( m_sortByURL, SIGNAL( toggled( bool ) ),
             this, SLOT( sortTypeChanged( bool ) ) );
    connect( m_sortByExtension, SIGNAL( toggled( bool ) ),
             this, SLOT( sortTypeChanged( bool ) ) );

    // tab sizes group box
    connect( m_tabSizesMinimumWidth, SIGNAL( valueChanged( int ) ),
             this, SLOT( minimumTabWidthChanged( int ) ) );
    connect( m_tabSizesMaximumWidth, SIGNAL( valueChanged( int ) ),
             this, SLOT( maximumTabWidthChanged( int ) ) );
    connect( m_tabSizesFixedHeight, SIGNAL( valueChanged( int ) ),
             this, SLOT( fixedTabHeightChanged( int ) ) );

    // button style group
    connect( m_buttonStylePush, SIGNAL( toggled( bool ) ),
             this, SLOT( buttonStyleChanged( bool ) ) );
    connect( m_buttonStyleFlat, SIGNAL( toggled( bool ) ),
             this, SLOT( buttonStyleChanged( bool ) ) );
    connect( m_buttonStylePlain, SIGNAL( toggled( bool ) ),
             this, SLOT( buttonStyleChanged( bool ) ) );

    // plain frame group
    connect( m_plainButtonColorsActivated, SIGNAL( changed( const QColor& ) ),
             this, SLOT( plainColorActivatedChanged( const QColor& ) ) );
    connect( m_plainButtonColorsHovered, SIGNAL( changed( const QColor& ) ),
             this, SLOT( plainColorHoveredChanged( const QColor& ) ) );
    connect( m_plainButtonColorsPressed, SIGNAL( changed( const QColor& ) ),
             this, SLOT( plainColorPressedChanged( const QColor& ) ) );

    // tab highlighting
    connect( m_highlightingModifiedTabs, SIGNAL( toggled( bool ) ),
             this, SLOT( highlightModifiedTabsChanged( bool ) ) );
    connect( m_highlightingModifiedTabsColor, SIGNAL( changed( const QColor& ) ),
             this, SLOT( modifiedTabsColorChanged( const QColor& ) ) );
    connect( m_highlightingActiveTab, SIGNAL( toggled( bool ) ),
             this, SLOT( highlightActiveTabChanged( bool ) ) );
    connect( m_highlightingPreviousTab, SIGNAL( toggled( bool ) ),
             this, SLOT( highlightPreviousTabChanged( bool ) ) );
    connect( m_highlightingActiveTabColor, SIGNAL( changed( const QColor& ) ),
             this, SLOT( activeTabColorChanged( const QColor& ) ) );
    connect( m_highlightingPreviousTabColor, SIGNAL( changed( const QColor& ) ),
             this, SLOT( previousTabColorChanged( const QColor& ) ) );
    connect( m_highlightingOpacity, SIGNAL( sliderMoved( int ) ),
             this, SLOT( highlightOpacityChanged( int ) ) );

    // preview
    connect( m_previewMinimum, SIGNAL( activated( KTinyTabButton* ) ),
             this, SLOT( buttonActivated( KTinyTabButton* ) ) );
    connect( m_previewMaximum, SIGNAL( activated( KTinyTabButton* ) ),
             this, SLOT( buttonActivated( KTinyTabButton* ) ) );
}

KTinyTabBarConfigPage::~KTinyTabBarConfigPage()
{
}

//BEGIN protected slots

void KTinyTabBarConfigPage::locationTopChanged( bool top )
{
    Q_UNUSED( top );
    emit changed();
}

void KTinyTabBarConfigPage::numberOfRowsChanged( int value )
{
    Q_UNUSED( value );
    emit changed();
}

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

void KTinyTabBarConfigPage::sortTypeChanged( bool checked )
{
    Q_UNUSED( checked );
    emit changed();
}

void KTinyTabBarConfigPage::buttonStyleChanged( bool checked )
{
    // slot called three times (3 radio buttons). only accept one of them.
    if( !checked )
        return;

    KTinyTabBar::ButtonStyle style = tabButtonStyle();
    m_previewMinimum->setTabButtonStyle( style );
    m_previewMaximum->setTabButtonStyle( style );
    emit changed();
}

void KTinyTabBarConfigPage::followCurrentTabChanged( bool follow )
{
    Q_UNUSED( follow );
    emit changed();
}

void KTinyTabBarConfigPage::plainColorActivatedChanged( const QColor &newColor )
{
    m_previewMinimum->setPlainColorActivated( newColor );
    m_previewMaximum->setPlainColorActivated( newColor );
    emit changed();
}

void KTinyTabBarConfigPage::plainColorHoveredChanged( const QColor &newColor )
{
    m_previewMinimum->setPlainColorHovered( newColor );
    m_previewMaximum->setPlainColorHovered( newColor );
    emit changed();
}

void KTinyTabBarConfigPage::plainColorPressedChanged( const QColor &newColor )
{
    m_previewMinimum->setPlainColorPressed( newColor );
    m_previewMaximum->setPlainColorPressed( newColor );
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
    m_highlightingOpacityLabel->setText(
            i18n( "&Opacity of highlighted button (%1%):", value ) );
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
    if( button == m_previewMinimum )
    {
        m_previewMinimum->setPreviousTab( false );
        m_previewMaximum->setActivated( false );
        m_previewMaximum->setPreviousTab( true );
    }
    else
    {
        m_previewMaximum->setPreviousTab( false );
        m_previewMinimum->setActivated( false );
        m_previewMinimum->setPreviousTab( true );
    }
}
//END protected slots

bool KTinyTabBarConfigPage::locationTop() const
{
    return m_locationTop->isChecked();
}

void KTinyTabBarConfigPage::setLocationTop( bool value )
{
    m_locationTop->setChecked( value );
    m_locationBottom->setChecked( !value );
}


int KTinyTabBarConfigPage::minimumTabWidth() const
{
    return m_tabSizesMinimumWidth->value();
}

void KTinyTabBarConfigPage::setMinimumTabWidth( int value )
{
    m_tabSizesMinimumWidth->setValue( value );
}

int KTinyTabBarConfigPage::maximumTabWidth() const
{
    return m_tabSizesMaximumWidth->value();
}

void KTinyTabBarConfigPage::setMaximumTabWidth( int value )
{
    m_tabSizesMaximumWidth->setValue( value );
}

int KTinyTabBarConfigPage::fixedTabHeight() const
{
    return m_tabSizesFixedHeight->value();
}

void KTinyTabBarConfigPage::setFixedTabHeight( int value )
{
    m_tabSizesFixedHeight->setValue( value );
}


int KTinyTabBarConfigPage::numberOfRows() const
{
    return m_rowsRows->value();
}

void KTinyTabBarConfigPage::setNumberOfRows( int value )
{
    m_rowsRows->setValue( value );
}

bool KTinyTabBarConfigPage::followCurrentTab() const
{
    return m_rowsFollowCurrentTab->isChecked();
}

void KTinyTabBarConfigPage::setFollowCurrentTab( bool value )
{
    m_rowsFollowCurrentTab->setChecked( value );
}

void KTinyTabBarConfigPage::setTabSortType( KTinyTabBar::SortType type )
{
    switch( type )
    {
        case KTinyTabBar::OpeningOrder: m_sortByOpening->setChecked( true ); break;
        case KTinyTabBar::Name: m_sortByName->setChecked( true ); break;
        case KTinyTabBar::URL: m_sortByURL->setChecked( true ); break;
        case KTinyTabBar::Extension: m_sortByExtension->setChecked( true ); break;
    }
}

KTinyTabBar::SortType KTinyTabBarConfigPage::tabSortType() const
{
    if( m_sortByOpening->isChecked() ) return KTinyTabBar::OpeningOrder;
    if( m_sortByName->isChecked() ) return KTinyTabBar::Name;
    if( m_sortByURL->isChecked() ) return KTinyTabBar::URL;
    return KTinyTabBar::Extension;
}


void KTinyTabBarConfigPage::setTabButtonStyle( KTinyTabBar::ButtonStyle style )
{
    switch( style )
    {
        case KTinyTabBar::Push: m_buttonStylePush->setChecked( true ); break;
        case KTinyTabBar::Flat: m_buttonStyleFlat->setChecked( true ); break;
        case KTinyTabBar::Plain:m_buttonStylePlain->setChecked( true ); break;
    }
}

KTinyTabBar::ButtonStyle KTinyTabBarConfigPage::tabButtonStyle() const
{
    if( m_buttonStylePush->isChecked() ) return KTinyTabBar::Push;
    if( m_buttonStyleFlat->isChecked() ) return KTinyTabBar::Flat;
    return KTinyTabBar::Plain;
}

void KTinyTabBarConfigPage::setPlainColorActivated( const QColor& color )
{
    m_plainButtonColorsActivated->setColor( color );
    m_previewMinimum->setPlainColorActivated( color );
    m_previewMaximum->setPlainColorActivated( color );
}

void KTinyTabBarConfigPage::setPlainColorHovered( const QColor& color )
{
    m_plainButtonColorsHovered->setColor( color );
    m_previewMinimum->setPlainColorHovered( color );
    m_previewMaximum->setPlainColorHovered( color );
}

void KTinyTabBarConfigPage::setPlainColorPressed( const QColor& color )
{
    m_plainButtonColorsPressed->setColor( color );
    m_previewMinimum->setPlainColorPressed( color );
    m_previewMaximum->setPlainColorPressed( color );
}

QColor KTinyTabBarConfigPage::plainColorActivated() const
{
    return m_plainButtonColorsActivated->color();
}

QColor KTinyTabBarConfigPage::plainColorHovered() const
{
    return m_plainButtonColorsHovered->color();
}

QColor KTinyTabBarConfigPage::plainColorPressed() const
{
    return m_plainButtonColorsPressed->color();
}


void KTinyTabBarConfigPage::setHighlightActiveTab( bool value )
{
    m_highlightingActiveTab->setChecked( value );
    m_previewMinimum->setHighlightActiveTab( value );
    m_previewMaximum->setHighlightActiveTab( value );
}

bool KTinyTabBarConfigPage::highlightActiveTab() const
{
    return m_highlightingActiveTab->isChecked();
}

void KTinyTabBarConfigPage::setActiveTabColor( const QColor& color )
{
    m_highlightingActiveTabColor->setColor( color );
    m_previewMinimum->setActiveTabColor( color );
    m_previewMaximum->setActiveTabColor( color );
}

QColor KTinyTabBarConfigPage::activeTabColor() const
{
    return m_highlightingActiveTabColor->color();
}


void KTinyTabBarConfigPage::setHighlightPreviousTab( bool value )
{
    m_highlightingPreviousTab->setChecked( value );
    m_previewMinimum->setHighlightPreviousTab( value );
    m_previewMaximum->setHighlightPreviousTab( value );
}

bool KTinyTabBarConfigPage::highlightPreviousTab() const
{
    return m_highlightingPreviousTab->isChecked();
}

void KTinyTabBarConfigPage::setPreviousTabColor( const QColor& color )
{
    m_highlightingPreviousTabColor->setColor( color );
    m_previewMinimum->setPreviousTabColor( color );
    m_previewMaximum->setPreviousTabColor( color );
}

QColor KTinyTabBarConfigPage::previousTabColor() const
{
    return m_highlightingPreviousTabColor->color();
}

void KTinyTabBarConfigPage::setHighlightOpacity( int value )
{
    m_highlightingOpacityLabel->setText(
            i18n( "&Opacity of highlighted button (%1%):", value ) );
    m_highlightingOpacity->setValue( value );
    m_previewMinimum->setHighlightOpacity( value );
    m_previewMaximum->setHighlightOpacity( value );
}

int KTinyTabBarConfigPage::highlightOpacity() const
{
    return m_highlightingOpacity->value();
}

void KTinyTabBarConfigPage::setHighlightModifiedTabs( bool modified )
{
    m_highlightingModifiedTabs->setChecked( modified );
    m_previewMinimum->setHighlightModifiedTabs( modified );
    m_previewMaximum->setHighlightModifiedTabs( modified );
}

bool KTinyTabBarConfigPage::highlightModifiedTabs() const
{
    return m_highlightingModifiedTabs->isChecked();
}

void KTinyTabBarConfigPage::setModifiedTabsColor( const QColor& color )
{
    m_highlightingModifiedTabsColor->setColor( color );
    m_previewMinimum->setModifiedTabsColor( color );
    m_previewMaximum->setModifiedTabsColor( color );
}

QColor KTinyTabBarConfigPage::modifiedTabsColor() const
{
    return m_highlightingModifiedTabsColor->color();
}



#include "ktinytabbarconfigpage.moc"

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs off; eol unix;
