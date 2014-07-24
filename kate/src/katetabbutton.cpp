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
#include <QPropertyAnimation>
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


QColor KateTabButton::s_predefinedColors[] = { Qt::red, Qt::yellow, Qt::green, Qt::cyan, Qt::blue, Qt::magenta };
const int KateTabButton::s_colorCount = 6;
int KateTabButton::s_currentColor = 0;

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
    } else if (m_highlightColor.isValid()) {
        p.setOpacity(0.3);
        p.fillRect(QRect(0, height() - 3, width(), 10), m_highlightColor);
        p.setOpacity(1.0);
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
    ev->accept();
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
    } else if (ev->button() == Qt::LeftButton) {
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
