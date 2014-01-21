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

#include "katetabbutton.h"

#include <klocalizedstring.h>

#include <QApplication>
#include <QColorDialog>
#include <QContextMenuEvent>
#include <QFontDatabase>
#include <QIcon>
#include <QMenu>
#include <QPainter>

QColor KateTabButton::s_predefinedColors[] = { Qt::red, Qt::yellow, Qt::green, Qt::cyan, Qt::blue, Qt::magenta };
const int KateTabButton::s_colorCount = 6;
int KateTabButton::s_currentColor = 0;


KateTabButton::KateTabButton(const QString &docurl, const QString &caption,
                             int button_id, QWidget *parent)
    : QPushButton(parent)
{
    setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    setCheckable(true);
    setFocusPolicy(Qt::NoFocus);
    setMinimumWidth(1);
    setFlat(true);
//     setAutoFillBackground(true);

    m_buttonId = button_id;
    m_modified = false;

    setIcon(QIcon());
    setText(caption);
    setURL(docurl);

    connect(this, SIGNAL(clicked()), this, SLOT(buttonClicked()));
}

KateTabButton::~KateTabButton()
{
}

void KateTabButton::setURL(const QString &docurl)
{
    m_url = docurl;
    if (!m_url.isEmpty()) {
        setToolTip(m_url);
    } else {
        setToolTip(text());
    }
}

QString KateTabButton::url() const
{
    return m_url;
}

void KateTabButton::buttonClicked()
{
    // once down, stay down until another tab is activated
    if (isChecked()) {
        emit activated(this);
    } else {
        setChecked(true);
    }
}

void KateTabButton::setActivated(bool active)
{
    if (isChecked() == active) {
        return;
    }
    setChecked(active);
    update();
}

bool KateTabButton::isActivated() const
{
    return isChecked();
}

void KateTabButton::paintEvent(QPaintEvent *ev)
{
    const int opac = 30;
    const int comp = 100 - opac;

    QPalette pal = QApplication::palette();
    if (m_highlightColor.isValid()) {
        QColor col(pal.button().color());
        col.setRed((col.red()*comp + m_highlightColor.red()*opac) / 100);
        col.setGreen((col.green()*comp + m_highlightColor.green()*opac) / 100);
        col.setBlue((col.blue()*comp + m_highlightColor.blue()*opac) / 100);
        pal.setColor(QPalette::Button, col);
        pal.setColor(QPalette::Background, col);
    }
    setPalette(pal);
    QPushButton::paintEvent(ev);
}

void KateTabButton::contextMenuEvent(QContextMenuEvent *ev)
{
    QPixmap colorIcon(22, 22);
    QMenu menu(/*text(),*/ this);
    QMenu *colorMenu = menu.addMenu(i18n("&Highlight Tab"));
    QAction *aNone = colorMenu->addAction(i18n("&None"));
    colorMenu->addSeparator();
    colorIcon.fill(Qt::red);
    QAction *aRed = colorMenu->addAction(colorIcon, i18n("&Red"));
    colorIcon.fill(Qt::yellow);
    QAction *aYellow = colorMenu->addAction(colorIcon, i18n("&Yellow"));
    colorIcon.fill(Qt::green);
    QAction *aGreen = colorMenu->addAction(colorIcon, i18n("&Green"));
    colorIcon.fill(Qt::cyan);
    QAction *aCyan = colorMenu->addAction(colorIcon, i18n("&Cyan"));
    colorIcon.fill(Qt::blue);
    QAction *aBlue = colorMenu->addAction(colorIcon, i18n("&Blue"));
    colorIcon.fill(Qt::magenta);
    QAction *aMagenta = colorMenu->addAction(colorIcon, i18n("&Magenta"));
    colorMenu->addSeparator();
    QAction *aCustomColor = colorMenu->addAction(
                                QIcon::fromTheme(QStringLiteral("colors")), i18n("C&ustom Color..."));
    menu.addSeparator();

    QAction *aCloseTab = menu.addAction(i18n("&Close Tab"));
    QAction *aCloseOtherTabs = menu.addAction(i18n("Close &Other Tabs"));
    QAction *aCloseAllTabs = menu.addAction(i18n("Close &All Tabs"));

    QAction *choice = menu.exec(ev->globalPos());

    // process the result
    if (choice == aNone) {
        if (m_highlightColor.isValid()) {
            setHighlightColor(QColor());
            emit highlightChanged(this);
        }
    } else if (choice == aRed) {
        setHighlightColor(Qt::red);
        emit highlightChanged(this);
    } else if (choice == aYellow) {
        setHighlightColor(Qt::yellow);
        emit highlightChanged(this);
    } else if (choice == aGreen) {
        setHighlightColor(Qt::green);
        emit highlightChanged(this);
    } else if (choice == aCyan) {
        setHighlightColor(Qt::cyan);
        emit highlightChanged(this);
    } else if (choice == aBlue) {
        setHighlightColor(Qt::blue);
        emit highlightChanged(this);
    } else if (choice == aMagenta) {
        setHighlightColor(Qt::magenta);
        emit highlightChanged(this);
    } else if (choice == aCustomColor) {
        QColor newColor = QColorDialog::getColor(m_highlightColor, this);
        if (newColor.isValid()) {
            setHighlightColor(newColor);
            emit highlightChanged(this);
        }
    } else if (choice == aCloseTab) {
        emit closeRequest(this);
    } else if (choice == aCloseOtherTabs) {
        emit closeOtherTabsRequest(this);
    } else if (choice == aCloseAllTabs) {
        emit closeAllTabsRequest();
    }
}

void KateTabButton::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::MidButton) {
        if (ev->modifiers() & Qt::ControlModifier) {
            // clear tab highlight
            setHighlightColor(QColor());
        } else {
            setHighlightColor(s_predefinedColors[s_currentColor]);
            if (++s_currentColor >= s_colorCount) {
                s_currentColor = 0;
            }
        }
        ev->accept();
    } else {
        QPushButton::mousePressEvent(ev);
    }
}

void KateTabButton::setButtonID(int button_id)
{
    m_buttonId = button_id;
}

int KateTabButton::buttonID() const
{
    return m_buttonId;
}

void KateTabButton::setHighlightColor(const QColor &color)
{
    if (color.isValid()) {
        m_highlightColor = color;
        update();
    } else if (m_highlightColor.isValid()) {
        m_highlightColor = QColor();
        update();
    }
}

QColor KateTabButton::highlightColor() const
{
    return m_highlightColor;
}

void KateTabButton::setModified(bool modified)
{
    m_modified = modified;
    update();
}

bool KateTabButton::isModified() const
{
    return m_modified;
}

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on; eol unix;
