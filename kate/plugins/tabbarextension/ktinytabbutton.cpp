/***************************************************************************
                           ktinytabbutton.cpp
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

#include "ktinytabbutton.h"
#include "ktinytabbutton.moc"

#include <kcolordialog.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>

#include <QApplication>
#include <QContextMenuEvent>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionButton>
#include <qdrawutil.h>

KTinyTabButton::KTinyTabButton( const QString& docurl, const QString& caption,
                                int button_id, bool blockContextMenu, QWidget *parent )
    : QPushButton( parent )
{
    setCheckable( true );
    setFocusPolicy( Qt::NoFocus );
    setMinimumWidth( 1 );

    if( blockContextMenu )
        setContextMenuPolicy( Qt::NoContextMenu );

    m_buttonId = button_id;
    m_tabButtonStyle = KTinyTabBar::Push;
    m_highlightModifiedTab = false;
    m_isPreviousTab = false;
    m_highlightColor = QColor(); // set to invalid color
    m_highlightActiveTab = false;
    m_highlightPreviousTab = false;
    m_highlightOpacity = 20;
    m_modified = false;

    setIcon( QIcon() );
    setText( caption );
    setURL( docurl );

    connect( this, SIGNAL( clicked() ), this, SLOT( buttonClicked() ) );
}

KTinyTabButton::~KTinyTabButton()
{
}

void KTinyTabButton::setURL( const QString& docurl )
{
    m_url = docurl;
    if( !m_url.isEmpty() )
        setToolTip( m_url );
    else
        setToolTip( text() );
}

QString KTinyTabButton::url() const
{
    return m_url;
}


void KTinyTabButton::buttonClicked()
{
    // once down, stay down until another tab is activated
    if( isChecked() )
        emit activated( this );
    else
        setChecked( true );
}

void KTinyTabButton::setActivated( bool active )
{
    if( isChecked() == active ) return;
    setChecked( active );
    update();
}

bool KTinyTabButton::isActivated() const
{
    return isChecked();
}

void KTinyTabButton::paintEvent( QPaintEvent* ev )
{
    const int opac = m_highlightOpacity;
    const int comp = 100 - opac;

    QColor mix = ( highlightActiveTab() && isActivated() ) ? m_colorActiveTab :
        ( ( highlightPreviousTab() && previousTab() ) ? m_colorPreviousTab :
            m_highlightColor );
    QPalette pal = QApplication::palette();
    if( isModified() && highlightModifiedTabs() )
        pal.setColor( QPalette::ButtonText, modifiedTabsColor() );


    setAutoFillBackground( false );
    switch( tabButtonStyle() )
    {
        case KTinyTabBar::Push:
        case KTinyTabBar::Flat:
        {
            if( m_highlightColor.isValid()
                || ( isActivated() && highlightActiveTab() )
                || ( previousTab() && highlightPreviousTab() ) )
            {
                setAutoFillBackground( true );
                QColor col( pal.button().color() );
                col.setRed( ( col.red()*comp + mix.red()*opac ) / 100 );
                col.setGreen( ( col.green()*comp + mix.green()*opac ) / 100 );
                col.setBlue( ( col.blue()*comp + mix.blue()*opac ) / 100 );
                pal.setColor( QPalette::Button, col );
                if( tabButtonStyle() == KTinyTabBar::Flat )
                    pal.setColor( QPalette::Background, col );
            }
            setPalette( pal );
            QPushButton::paintEvent( ev );

            break; 
        }

        case KTinyTabBar::Plain:
        {
            bool up = false;
            QPainter p( this );
            if( isActivated() )
                p.setPen( m_plainColorActivated );
            else if( isDown() )
                p.setPen( m_plainColorPressed );
            else if( underMouse() )
                p.setPen( m_plainColorHovered );
            else
                up = true;

            QColor bg = QApplication::palette().background().color();
            if( m_highlightColor.isValid()
                || ( isActivated() && highlightActiveTab() )
                || ( previousTab() && highlightPreviousTab() ) )
            {
                bg.setRed( ( bg.red()*comp + mix.red()*opac ) / 100 );
                bg.setGreen( ( bg.green()*comp + mix.green()*opac ) / 100 );
                bg.setBlue( ( bg.blue()*comp + mix.blue()*opac ) / 100 );
            }
            // fill background
            p.fillRect( rect(), bg );

            if( up )
            {
                // the button is not activated, draw raised panel
                qDrawShadePanel( &p, 0, 0, width(), height(), pal );
            }
            else
            {
                // draw frame
                p.drawRect( 0, 0, width() - 1, height() - 1 );
//                 p.drawRect( 1, 1, width() - 3, height() - 3 );
            }

            QStyleOptionButton opt;
            opt.init( this );
            opt.features = QStyleOptionButton::None;
            if( isFlat() )
                opt.features |= QStyleOptionButton::Flat;
            if( isDefault() )
                opt.features |= QStyleOptionButton::DefaultButton;
            if( isDown() )
                opt.state |= QStyle::State_Sunken;
            if( isChecked() )
                opt.state |= QStyle::State_On;
            if( !isFlat() && !isDown() )
                opt.state |= QStyle::State_Raised;
            opt.text = text();
            opt.icon = icon();
            opt.iconSize = QSize( 16, 16 );

            // draw icon and text
            setPalette( pal );
            style()->drawControl( QStyle::CE_PushButtonLabel, &opt, &p );
            break;
        }
    }
}

void KTinyTabButton::contextMenuEvent( QContextMenuEvent* ev )
{
    QPixmap colorIcon( 22, 22 );
    QMenu menu( /*text(),*/ this );
    QMenu* colorMenu = menu.addMenu( i18n( "&Highlight Tab" ) );
    QAction* aNone = colorMenu->addAction( i18n( "&None" ) );
    colorMenu->addSeparator();
    colorIcon.fill( Qt::red );
    QAction* aRed = colorMenu->addAction( colorIcon, i18n( "&Red" ) );
    colorIcon.fill( Qt::yellow );
    QAction* aYellow = colorMenu->addAction( colorIcon, i18n( "&Yellow" ) );
    colorIcon.fill( Qt::green );
    QAction* aGreen = colorMenu->addAction( colorIcon, i18n( "&Green" ) );
    colorIcon.fill( Qt::cyan );
    QAction* aCyan = colorMenu->addAction( colorIcon, i18n( "&Cyan" ) );
    colorIcon.fill( Qt::blue );
    QAction* aBlue = colorMenu->addAction( colorIcon, i18n( "&Blue" ) );
    colorIcon.fill( Qt::magenta );
    QAction* aMagenta = colorMenu->addAction( colorIcon, i18n( "&Magenta" ) );
    colorMenu->addSeparator();
    QAction* aCustomColor = colorMenu->addAction(
        QIcon( SmallIcon( "colors" ) ), i18n( "C&ustom Color..." ) );
    menu.addSeparator();

    QAction* aCloseTab = menu.addAction( i18n( "&Close Tab" ) );
    QAction* aCloseOtherTabs = menu.addAction( i18n( "Close &Other Tabs" ) );
    QAction* aCloseAllTabs = menu.addAction( i18n( "Close &All Tabs" ) );

    QAction* choice = menu.exec( ev->globalPos() );

    // process the result
    if( choice == aNone ) {
        if( m_highlightColor.isValid() )
        {
            setHighlightColor( QColor() );
            emit highlightChanged( this );
        }
    } else if( choice == aRed ) {
        setHighlightColor( Qt::red );
        emit highlightChanged( this );
    } else if( choice == aYellow ) {
        setHighlightColor( Qt::yellow );
        emit highlightChanged( this );
    } else if( choice == aGreen ) {
        setHighlightColor( Qt::green );
        emit highlightChanged( this );
    } else if( choice == aCyan ) {
        setHighlightColor( Qt::cyan );
        emit highlightChanged( this );
    } else if( choice == aBlue ) {
        setHighlightColor( Qt::blue );
        emit highlightChanged( this );
    } else if( choice == aMagenta ) {
        setHighlightColor( Qt::magenta );
        emit highlightChanged( this );
    } else if( choice == aCustomColor ) {
        QColor newColor;
        int result = KColorDialog::getColor( newColor, m_highlightColor, this );
        if ( result == KColorDialog::Accepted )
        {
            setHighlightColor( newColor );
            emit highlightChanged( this );
        }
    } else if( choice == aCloseTab ) {
        emit closeRequest( this );
    } else if (choice == aCloseOtherTabs) {
        emit closeOtherTabsRequest (this);
    } else if (choice == aCloseAllTabs) {
        emit closeAllTabsRequest ();
    }
    
}

void KTinyTabButton::setButtonID( int button_id )
{
    m_buttonId = button_id;
}

int KTinyTabButton::buttonID() const
{
    return m_buttonId;
}

void KTinyTabButton::setHighlightColor( const QColor& color )
{
    if( color.isValid() )
    {
        m_highlightColor = color;
        update();
    }
    else if( m_highlightColor.isValid() )
    {
        m_highlightColor = QColor();
        update();
    }
}

QColor KTinyTabButton::highlightColor() const
{
    return m_highlightColor;
}

void KTinyTabButton::setTabButtonStyle( KTinyTabBar::ButtonStyle tabStyle )
{
    if( m_tabButtonStyle == tabStyle )
        return;

    setFlat( tabStyle == KTinyTabBar::Flat );
    m_tabButtonStyle = tabStyle;
    update();
}

KTinyTabBar::ButtonStyle KTinyTabButton::tabButtonStyle() const
{
    return m_tabButtonStyle;
}

void KTinyTabButton::setPreviousTab( bool previous )
{
    m_isPreviousTab = previous;
    update();
}

bool KTinyTabButton::previousTab() const
{
    return m_isPreviousTab;
}

void KTinyTabButton::setHighlightOpacity( int value )
{
    m_highlightOpacity = value;
    update();
}

int KTinyTabButton::highlightOpacity() const
{
    return m_highlightOpacity;
}

void KTinyTabButton::setHighlightActiveTab( bool value )
{
    m_highlightActiveTab = value;
    update();
}

bool KTinyTabButton::highlightActiveTab()
{
    return m_highlightActiveTab;
}

void KTinyTabButton::setHighlightPreviousTab( bool value )
{
    m_highlightPreviousTab = value;
    update();
}

bool KTinyTabButton::highlightPreviousTab()
{
    return m_highlightPreviousTab;
}


void KTinyTabButton::setPlainColorPressed( const QColor& color )
{
    m_plainColorPressed = color;
    if( tabButtonStyle() == KTinyTabBar::Plain )
        update();
}

QColor KTinyTabButton::plainColorPressed() const
{
    return m_plainColorPressed;
}

void KTinyTabButton::setPlainColorHovered( const QColor& color )
{
    m_plainColorHovered= color;
    if( tabButtonStyle() == KTinyTabBar::Plain )
        update();
}

QColor KTinyTabButton::plainColorHovered() const
{
    return m_plainColorHovered;
}

void KTinyTabButton::setPlainColorActivated( const QColor& color )
{
    m_plainColorActivated = color;
    if( tabButtonStyle() == KTinyTabBar::Plain )
        update();
}

QColor KTinyTabButton::plainColorActivated() const
{
    return m_plainColorActivated;
}

void KTinyTabButton::setPreviousTabColor( const QColor& color )
{
    m_colorPreviousTab = color;
    if( highlightPreviousTab() )
        update();
}

QColor KTinyTabButton::previousTabColor() const
{
    return m_colorPreviousTab;
}

void KTinyTabButton::setActiveTabColor( const QColor& color )
{
    m_colorActiveTab = color;
    if( isActivated() )
        update();
}

QColor KTinyTabButton::activeTabColor() const
{
    return m_colorActiveTab;
}

void KTinyTabButton::setHighlightModifiedTabs( bool highlight )
{
    m_highlightModifiedTab = highlight;
    if( isModified() )
        update();
}

bool KTinyTabButton::highlightModifiedTabs() const
{
    return m_highlightModifiedTab;
}

void KTinyTabButton::setModifiedTabsColor( const QColor& color )
{
    m_colorModifiedTab = color;
    if( isModified() )
        update();
}

QColor KTinyTabButton::modifiedTabsColor() const
{
    return m_colorModifiedTab;
}


void KTinyTabButton::setModified( bool modified )
{
    m_modified = modified;
    update();
}

bool KTinyTabButton::isModified() const
{
    return m_modified;
}

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs off; eol unix;
