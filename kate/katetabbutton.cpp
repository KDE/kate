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

#include <KLocalizedString>

#include <QApplication>
// #include <QDebug>
#include <QDrag>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QMimeData>
#include <QMoveEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyle>
#include <QStyleOption>

#include <math.h>

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
    const int w = style()->pixelMetric(QStyle::PM_TabCloseIndicatorWidth, nullptr, this);
    const int h = style()->pixelMetric(QStyle::PM_TabCloseIndicatorHeight, nullptr, this);
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
    , m_geometryAnimation(nullptr)
{
    setCheckable(true);
    setFocusPolicy(Qt::NoFocus);

    setText(text);

    // add close button
    const int margin = style()->pixelMetric(QStyle::PM_ButtonMargin, nullptr, this);
    m_closeButton = new TabCloseButton(this);
    QHBoxLayout * hbox = new QHBoxLayout(this);
    hbox->setSpacing(0);
    hbox->setContentsMargins(margin, 0, margin, 0);
    hbox->addStretch();
    hbox->addWidget(m_closeButton);
    setLayout(hbox);
    connect(m_closeButton, &TabCloseButton::clicked, this, &KateTabButton::closeButtonClicked);
}

void KateTabButton::closeButtonClicked()
{
    emit closeRequest(this);
}

void KateTabButton::paintEvent(QPaintEvent *ev)
{
    Q_UNUSED(ev)

    QColor barColor(palette().color(QPalette::Highlight));

    // read from the parent widget (=KateTabBar) the isActiveTabBar property
    if (!isActiveTabBar()) {
        // if inactive, convert color to gray value
        const int g = qGray(barColor.rgb());
        barColor = QColor(g, g, g);
    }

    // compute sane margins
    const int margin = style()->pixelMetric(QStyle::PM_ButtonMargin, nullptr, this);
    const int barMargin = margin / 2;
    const int barHeight = ceil(height() / 10.0);

    QPainter p(this);

    // paint bar if inactive but hovered
    if (!isChecked() && underMouse()) {
        barColor.setAlpha(80);
        p.fillRect(QRect(barMargin, height() - barHeight, width() - 2 * barMargin, barHeight), barColor);
    }

    // paint bar
    if (isChecked()) {
        barColor.setAlpha(255);
        p.fillRect(QRect(barMargin, height() - barHeight, width() - 2 * barMargin, barHeight), barColor);
    }

    // icon, if applicable
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
    style()->drawItemText(&p, textRect, Qt::AlignHCenter | Qt::AlignVCenter, pal, true, elidedText, QPalette::WindowText);
}

void KateTabButton::mousePressEvent(QMouseEvent *ev)
{
    ev->accept();

    // save mouse position for possible drag start event
    m_mouseDownPosition = ev->globalPos();

    // activation code
    if (ev->button() == Qt::LeftButton) {
        if (! isChecked()) {
            // make sure we stay checked
            setChecked(true);
        }

        // notify that this button was activated
        emit activated(this);
    } else if (ev->button() == Qt::MiddleButton) {
        emit closeRequest(this);
    } else {
        ev->ignore();
    }
}

void KateTabButton::mouseMoveEvent(QMouseEvent *event)
{
    // possibly start drag event
    if (QPoint(event->globalPos() - m_mouseDownPosition).manhattanLength() > QApplication::startDragDistance())
    {
        QMimeData *mimeData = new QMimeData;
        mimeData->setData(QStringLiteral("application/x-dndkatetabbutton"), QByteArray());
        mimeData->setUrls({m_url});

        auto drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->setDragCursor(QPixmap(), Qt::MoveAction);
        drag->start(Qt::MoveAction);
        event->accept();
    }
    QAbstractButton::mouseMoveEvent(event);
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

void KateTabButton::moveEvent(QMoveEvent *event)
{
    // tell the tabbar to redraw its separators. Since the separators overlap
    // the tab buttons geometry, we need to adjust the width by the separator's
    // width to avoid artifacts
    if (parentWidget()) {
        const int w = style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent, nullptr, this);
        QRect rect = geometry();
        rect.moveLeft(event->oldPos().x());
        rect.adjust(-w, 0, w, 0);
        parentWidget()->update(rect);
    }
    QAbstractButton::moveEvent(event);
}

bool KateTabButton::isActiveTabBar() const
{
    Q_ASSERT(parentWidget());

    // read from the parent widget (=KateTabBar) the isActive property
    return parentWidget()->property("isActive").toBool();
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
    if (! style()->styleHint(QStyle::SH_Widget_Animate, nullptr, this)
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

bool KateTabButton::geometryAnimationRunning() const
{
    return m_geometryAnimation
        && (m_geometryAnimation->state() != QAbstractAnimation::Stopped);
}

QUrl KateTabButton::url() const
{
    return m_url;
}

void KateTabButton::setUrl(const QUrl &url)
{
    m_url = url;
}
