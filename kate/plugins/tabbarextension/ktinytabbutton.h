/***************************************************************************
                           ktinytabbutton.h
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

#ifndef TINYTABBUTTON_H
#define TINYTABBUTTON_H

#include <QPushButton>
#include "ktinytabbar.h"


/**
 * A \p KTinyTabButton represents a button on the tab bar. It can either be
 * \e activated or \e deactivated. If the state is \e deactivated it will
 * be @e activated when the mouse is pressed. It then emits the signal
 * @p activated(). The \p KTinyTabButton's caption can be set with \p setText()
 * and an additional pixmap can be shown with \p setPixmap().
 *
 * @author Dominik Haumann
 */
class KTinyTabButton : public QPushButton
{
Q_OBJECT

public:
    /**
     * Constructs a new tab bar button with \a caption and \a parent. If
     * \e blockContextMenu is \e false the context menu will be blocked, i.e.
     * hidden. If the @p docurl is unknown, pass QString().
     */
    KTinyTabButton( const QString& docurl, const QString& caption, int button_id,
                    bool blockContextMenu = true, QWidget *parent = 0 );

    virtual ~KTinyTabButton();

    /**
     * Activate or deactivate the button. If the button is already down
     * and \a active is \e true nothing happens, otherwise the state toggles.
     * \note The signal \p activated is \e not emitted.
     */
    void setActivated( bool active );

    /**
     * Check the button status. The return value is \e true, if the button is
     * down.
     */
    bool isActivated() const;

    /**
     * Set a unique button id number.
     */
    void setButtonID( int button_id );

    /**
     * Get the unique id number.
     */
    int buttonID() const;

    /**
     * Set the document's url to @p docurl. If unknown, pass QString().
     */
    void setURL( const QString& docurl );

    /**
     * Get the document's url.
     */
    QString url() const;

    /**
     * Set the highlighted state. If @p color.isValid() is \e false the
     * button is not highlighted. This does \e not emit the signal
     * @p highlightChanged().
     * @param color the color
     */
    void setHighlightColor( const QColor& color );

    /**
     * Get the highlight color. If the button is not highlighted then the color
     * is invalid, i.e. \p QColor::isValid() returns \e flase.
     */
    QColor highlightColor() const;

    /**
     * Set the highlight opacity to @p value.
     * @param value opacity between 0..100
     */
    void setHighlightOpacity( int value );

    /**
     * Get the highlight opacity
     * @return opacity
     */
    int highlightOpacity() const;

    /**
     * Set whether the tab was previously selected.
     * @param previous truth vlaue
     */
    void setPreviousTab( bool previous );

    /**
     * Check whether the tab is the previously selected tab.
     * @return true, if tab was active before the current active tab
     */
    bool previousTab() const;

    /**
     * Set the tabbutton style.
     * @param tabStyle button style
     */
    void setTabButtonStyle( KTinyTabBar::ButtonStyle tabStyle );

    /**
     * Get the tabbutton style
     * @return button style
     */
    KTinyTabBar::ButtonStyle tabButtonStyle() const;

    /**
     * Set whether an active tab should be highlighted.
     */
    void setHighlightActiveTab( bool value );
    /**
     * Get whether an active tab should be highlighted.
     */
    bool highlightActiveTab();

    /**
     * Set whether a previous tab should be highlighted.
     */
    void setHighlightPreviousTab( bool value );
    /**
     * Get whether a previous tab is highlighted.
     */
    bool highlightPreviousTab();

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

    void setHighlightModifiedTabs( bool highlight );
    bool highlightModifiedTabs() const;

    void setModifiedTabsColor( const QColor& color );
    QColor modifiedTabsColor() const;

    void setModified( bool modified );
    bool isModified() const;

signals:
    /**
     * Emitted whenever the button changes state from deactivated to activated.
     * @param tabbutton the pressed button (this)
     */
    void activated( KTinyTabButton* tabbutton );

    /**
     * Emitted whenever the user changes the highlighted state. This can be
     * done only via the context menu.
     * @param tabbutton the changed button (this)
     */
    void highlightChanged( KTinyTabButton* tabbutton );

    /**
     * Emitted whenever the user wants to close the tab button.
     * @param tabbutton the button that emitted this signal
     */
    void closeRequest( KTinyTabButton* tabbutton );
    
    /**
     * Emitted whenever the user wants to close all the tab button except the
     * selected one.
     * @param tabbutton the button that emitted this signal
     */
    void closeOtherTabsRequest( KTinyTabButton* tabbutton );
    
    /**
     * Emitted whenever the user wants to close all the tabs.
     */
    void closeAllTabsRequest();

protected slots:
    void buttonClicked();

protected:
    /** paint eyecandy rectangles around the button */
    virtual void paintEvent( QPaintEvent* ev );
    /** support for context menu */
    virtual void contextMenuEvent( QContextMenuEvent* ev );

private:
    QString m_url;
    int m_buttonId;
    bool m_modified;
    bool m_highlightModifiedTab;
    bool m_highlightActiveTab;
    bool m_highlightPreviousTab;
    bool m_isPreviousTab;

    QColor m_plainColorPressed;
    QColor m_plainColorHovered;
    QColor m_plainColorActivated;
    QColor m_colorModifiedTab;
    QColor m_colorActiveTab;
    QColor m_colorPreviousTab;

    QColor m_highlightColor;
    KTinyTabBar::ButtonStyle m_tabButtonStyle;
    int m_highlightOpacity;
};

#endif

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs off; eol unix;
