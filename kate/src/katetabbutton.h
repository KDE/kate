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

#include <QAbstractButton>

class QPropertyAnimation;

class TabCloseButton : public QAbstractButton
{
    Q_OBJECT

public:
    // constructor
    TabCloseButton(QWidget * parent = 0);
    // paint close button depending on its state
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    // returns the size hint depending on the style
    QSize sizeHint() const Q_DECL_OVERRIDE;

protected:
    void enterEvent(QEvent *event) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *event) Q_DECL_OVERRIDE;
};

/**
 * A \p KateTabButton represents a button on the tab bar. It can either be
 * \e activated or \e deactivated. If the state is \e deactivated it will
 * be @e activated when the mouse is pressed. It then emits the signal
 * @p activated(). The \p KateTabButton's text can be set with \p setText()
 * and an additional icon can be shown with \p setIcon().
 *
 * @author Dominik Haumann
 */
class KateTabButton : public QAbstractButton
{
    Q_OBJECT

public:
    /**
     * Constructs a new tab bar button with \a text and \a parent.
     */
    KateTabButton(const QString &text, QWidget *parent = 0);

    virtual ~KateTabButton();

    /**
     * Returns @e true, if the tabbar is the currently active tab bar.
     */
    bool isActiveViewSpace() const;

    /**
     * Check whether a geometry animation is running.
     */
    bool geometryAnimationRunning() const;

public Q_SLOTS:
    /**
     * Animate the button's geometry from @p startGeom to @p endGeom
     * with linear interpolation.
     */
    void setAnimatedGeometry(const QRect & startGeom, const QRect & endGeom);

Q_SIGNALS:
    /**
     * Emitted whenever the button changes state from deactivated to activated,
     * or when the button was clicked although it was already active.
     * @param tabbutton the pressed button (this)
     */
    void activated(KateTabButton *tabbutton);

    /**
     * Emitted whenever the user wants to close the tab button.
     * @param tabbutton the button that emitted this signal
     */
    void closeRequest(KateTabButton *tabbutton);

protected Q_SLOTS:
    void closeButtonClicked();

protected:
    /** paint eyecandy rectangles around the button */
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    /** middle mouse button changes color */
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    /** eat double click events */
    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    /** trigger repaint on hover enter event */
    void enterEvent(QEvent *event) Q_DECL_OVERRIDE;
    /** trigger repaint on hover leave event */
    void leaveEvent(QEvent *event) Q_DECL_OVERRIDE;
    /** track geometry changes to trigger proper repaint*/
    void moveEvent(QMoveEvent *event) Q_DECL_OVERRIDE;

private:
    TabCloseButton * m_closeButton;
    QPropertyAnimation * m_geometryAnimation;
};

#endif

