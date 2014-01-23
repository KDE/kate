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

#ifndef KATE_TAB_BUTTON
#define KATE_TAB_BUTTON

#include <QPushButton>
#include "katetabbar.h"

/**
 * A \p KateTabButton represents a button on the tab bar. It can either be
 * \e activated or \e deactivated. If the state is \e deactivated it will
 * be @e activated when the mouse is pressed. It then emits the signal
 * @p activated(). The \p KateTabButton's caption can be set with \p setText()
 * and an additional pixmap can be shown with \p setPixmap().
 *
 * @author Dominik Haumann
 */
class KateTabButton : public QPushButton
{
    Q_OBJECT

public:
    /**
     * Constructs a new tab bar button with \a caption and \a parent.
     */
    KateTabButton(const QString &caption, int button_id, QWidget *parent = 0);

    virtual ~KateTabButton();

    /**
     * Activate or deactivate the button. If the button is already down
     * and \a active is \e true nothing happens, otherwise the state toggles.
     * \note The signal \p activated is \e not emitted.
     */
    void setActivated(bool active);

    /**
     * Check the button status. The return value is \e true, if the button is
     * down.
     */
    bool isActivated() const;

    /**
     * Set a unique button id number.
     */
    void setButtonID(int button_id);

    /**
     * Get the unique id number.
     */
    int buttonID() const;

    /**
     * Set the highlighted state. If @p color.isValid() is \e false the
     * button is not highlighted. This does \e not emit the signal
     * @p highlightChanged().
     * @param color the color
     */
    void setHighlightColor(const QColor &color);

    /**
     * Get the highlight color. If the button is not highlighted then the color
     * is invalid, i.e. \p QColor::isValid() returns \e flase.
     */
    QColor highlightColor() const;

    void setModified(bool modified);
    bool isModified() const;

Q_SIGNALS:
    /**
     * Emitted whenever the button changes state from deactivated to activated.
     * @param tabbutton the pressed button (this)
     */
    void activated(KateTabButton *tabbutton);

    /**
     * Emitted whenever the user changes the highlighted state. This can be
     * done only via the context menu.
     * @param tabbutton the changed button (this)
     */
    void highlightChanged(KateTabButton *tabbutton);

    /**
     * Emitted whenever the user wants to close the tab button.
     * @param tabbutton the button that emitted this signal
     */
    void closeRequest(KateTabButton *tabbutton);

    /**
     * Emitted whenever the user wants to close all the tab button except the
     * selected one.
     * @param tabbutton the button that emitted this signal
     */
    void closeOtherTabsRequest(KateTabButton *tabbutton);

    /**
     * Emitted whenever the user wants to close all the tabs.
     */
    void closeAllTabsRequest();

protected Q_SLOTS:
    void buttonClicked();

protected:
    /** paint eyecandy rectangles around the button */
    virtual void paintEvent(QPaintEvent *ev);
    /** support for context menu */
    virtual void contextMenuEvent(QContextMenuEvent *ev);
    /** middle mouse button changes color */
    virtual void mousePressEvent(QMouseEvent *ev);


private:
    int m_buttonId;
    bool m_modified;

    QColor m_highlightColor;

    static QColor s_predefinedColors[6];
    static const int s_colorCount;
    static int s_currentColor;
};

#endif

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on; eol unix;
