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
#include <QStyle>
#include <QStyleOption>
#include <QHBoxLayout>

TabCloseButton::TabCloseButton(QWidget * parent)
    : QAbstractButton(parent)
{
    // should never have focus
    setFocusPolicy(Qt::NoFocus);

    // closing a tab closes the document
    setToolTip(i18n("Close Document"));
}

void TabCloseButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    // get the tab this close button belongs to
    KateTabButton *tabButton = qobject_cast<KateTabButton*>(parent());
    const bool isActive = underMouse()
        || (tabButton && tabButton->isChecked());

    // set style options depending on current state
    QStyleOption opt;
    opt.init(this);
    if (isActive && !isDown()) {
        opt.state |= QStyle::State_Raised;
    }
    if (isDown()) {
        opt.state |= QStyle::State_Sunken;
    }

    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_IndicatorTabClose, &opt, &p, this);
}

QSize TabCloseButton::sizeHint() const
{
    // make sure the widget is polished
    ensurePolished();

    // read the metrics from the style
    const int w = style()->pixelMetric(QStyle::PM_TabCloseIndicatorWidth, 0, this);
    const int h = style()->pixelMetric(QStyle::PM_TabCloseIndicatorHeight, 0, this);
    return QSize(w, h);
}

void TabCloseButton::enterEvent(QEvent *event)
{
    update(); // repaint on hover
    QAbstractButton::enterEvent(event);
}

void TabCloseButton::leaveEvent(QEvent *event)
{
    update(); // repaint on hover
    QAbstractButton::leaveEvent(event);
}


QColor KateTabButton::s_predefinedColors[] = { Qt::red, Qt::yellow, Qt::green, Qt::cyan, Qt::blue, Qt::magenta };
const int KateTabButton::s_colorCount = 6;
int KateTabButton::s_currentColor = 0;

KateTabButton::KateTabButton(const QString &caption, QWidget *parent)
    : QPushButton(parent)
{
    setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    setCheckable(true);
    setFocusPolicy(Qt::NoFocus);
    setMinimumWidth(1);
    setFlat(true);

    m_modified = false;

    setIcon(QIcon());
    setText(caption);

    connect(this, SIGNAL(clicked()), this, SLOT(buttonClicked()));

    // add close button
    const int margin = style()->pixelMetric(QStyle::PM_ButtonMargin, 0, this);
    m_closeButton = new TabCloseButton(this);
    QHBoxLayout * hbox = new QHBoxLayout(this);
    hbox->setSpacing(0);
    hbox->setContentsMargins(0, 0, margin, 0);
    hbox->addStretch();
    hbox->addWidget(m_closeButton);
    setLayout(hbox);
    connect(m_closeButton, SIGNAL(clicked()), this, SLOT(closeButtonClicked()));
}

KateTabButton::~KateTabButton()
{
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

void KateTabButton::closeButtonClicked()
{
    emit closeRequest(this);
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
    Q_UNUSED(ev)

    QPalette pal = QApplication::palette();

    QPainter p(this);
    if (isChecked() || underMouse()) {
        QColor c = pal.color(QPalette::Background);
        p.fillRect(rect(), c.lighter(110));
    }

    if (m_highlightColor.isValid()) {
        p.fillRect(QRect(0, height() - 3, width(), 10), m_highlightColor);
    }

    if (isActivated()) {
        p.fillRect(QRect(0, height() - 3, width(), 10), QColor(0, 0, 255, 128));
    }

    // the width of the text is reduced by the close button + 2 * margin
    const int margin = style()->pixelMetric(QStyle::PM_ButtonMargin, 0, this);
    const int w = width() - m_closeButton->width() - 2 * margin;

    // draw text, we need to elide to xxx...xxx is too long
    const QString elidedText = QFontMetrics(font()).elidedText (text(), Qt::ElideMiddle, w);
    const QRect textRect(0, 0, w, height());
    style()->drawItemText(&p, textRect, Qt::AlignHCenter | Qt::AlignVCenter, pal, true, elidedText);
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

    QAction *aCloseTab = menu.addAction(i18n("&Close Document"));

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

