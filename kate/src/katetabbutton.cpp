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

#include "kateapp.h"
#include "katedocmanager.h"

#include <KLocalizedString>

#include <QApplication>
#include <QContextMenuEvent>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QMenu>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyle>
#include <QStyleOption>

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
    if (isActive && !isChecked()) {
        opt.state |= QStyle::State_Raised;
    }
    if (isChecked()) {
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


KateTabButton::KateTabButton(const QString &text, QWidget *parent)
    : QAbstractButton(parent)
    , m_geometryAnimation(0)
{
    setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    setCheckable(true);
    setFocusPolicy(Qt::NoFocus);

    setText(text);

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

void KateTabButton::closeButtonClicked()
{
    emit closeRequest(this);
}

void KateTabButton::paintEvent(QPaintEvent *ev)
{
    Q_UNUSED(ev)

    QColor barColor(palette().color(QPalette::Highlight));

    // read from the parent widget (=KateTabBar) the isActiveViewSpace property
    if (isActiveViewSpace()) {
        // if inactive, convert color to gray value
        const int g = qGray(barColor.rgb());
        barColor = QColor(g, g, g);
    }

    QPainter p(this);

    // paint background rect
    if (isChecked() || underMouse()) {
        QStyleOptionViewItemV4 option;
        option.initFrom(this);
        barColor.setAlpha(50);
        option.backgroundBrush = barColor;
        option.state = QStyle::State_Enabled | QStyle::State_MouseOver;
        option.viewItemPosition = QStyleOptionViewItemV4::OnlyOne;
        style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, &p, this);
    }

    // paint bar
    if (isChecked()) {
        barColor.setAlpha(255);
        p.fillRect(QRect(0, height() - 3, width(), 10), barColor);
    }

    // icon, if applicable
    const int margin = style()->pixelMetric(QStyle::PM_ButtonMargin, 0, this);
    int leftMargin = margin;
    if (! icon().isNull()) {
        const int y = (height() - 16) / 2;
        icon().paint(&p, margin, y, 16, 16);
        leftMargin += 16;
        leftMargin += margin;
    }

    // the width of the text is reduced by the close button + 2 * margin
    const int w = width() // width of widget
                - m_closeButton->width() - 2 * margin // close button
                - leftMargin; // modified button

    // draw text, we need to elide to xxx...xxx is too long
    const QString elidedText = QFontMetrics(font()).elidedText (text(), Qt::ElideMiddle, w);
    const QRect textRect(leftMargin, 0, w, height());
    const QPalette pal = QApplication::palette();
    style()->drawItemText(&p, textRect, Qt::AlignHCenter | Qt::AlignVCenter, pal, true, elidedText);
}

void KateTabButton::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu(/*text(),*/ this);
    QAction *aCloseTab = menu.addAction(i18n("&Close Document"));
    QAction *aCloseOthers = menu.addAction(i18n("&Close Other Documents"));
    if (KateApp::self()->documentManager()->documentList().count() < 2) {
        aCloseOthers->setEnabled(false);
    }

    QAction *choice = menu.exec(ev->globalPos());

    if (choice == aCloseTab) {
        emit closeRequest(this);
    } else if (choice == aCloseOthers) {
        emit closeOthersRequest(this);
    }
}

void KateTabButton::mousePressEvent(QMouseEvent *ev)
{
    ev->accept();
    if (ev->button() == Qt::LeftButton) {
        if (! isChecked()) {
            // make sure we stay checked
            setChecked(true);
        }

        // notify that this button was activated
        emit activated(this);
    } else {
        ev->ignore();
    }
}

void KateTabButton::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->accept();
}

void KateTabButton::enterEvent(QEvent *event)
{
    update(); // repaint on hover
    QAbstractButton::enterEvent(event);
}

void KateTabButton::leaveEvent(QEvent *event)
{
    update(); // repaint on hover
    QAbstractButton::leaveEvent(event);
}

bool KateTabButton::isActiveViewSpace() const
{
    Q_ASSERT(parentWidget());

    // read from the parent widget (=KateTabBar) the isActiveViewSpace property
    return ! parentWidget()->property("isActiveViewSpace").toBool();
}

void KateTabButton::setAnimatedGeometry(const QRect & startGeom,
                                        const QRect & endGeom)
{
    // stop animation in case it is running
    if (m_geometryAnimation &&
        m_geometryAnimation->state() != QAbstractAnimation::Stopped) {
        m_geometryAnimation->stop();
    }

    // already at desired position
    if (startGeom == geometry() && endGeom == startGeom) {
        return;
    }

    // if the style does not want animations, just set geometry
    if (! style()->styleHint(QStyle::SH_Widget_Animate, 0, this)
        || (isVisible() && endGeom == startGeom))
    {
        setGeometry(endGeom);
        return;
    }

    if (! m_geometryAnimation) {
        m_geometryAnimation = new QPropertyAnimation(this, "geometry", this);
        m_geometryAnimation->setDuration(100);
    }

    // finally start geometry animation
    m_geometryAnimation->setStartValue(startGeom);
    m_geometryAnimation->setEndValue(endGeom);
    m_geometryAnimation->start();
}
