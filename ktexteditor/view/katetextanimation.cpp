/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2013 Dominik Haumann <dhaumann kde org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */
#include "katetextanimation.h"

#include "kateviewinternal.h"
#include "katerenderer.h"

#include <ktexteditor/document.h>

#include <QTimeLine>
#include <QPainter>
#include <QRect>
#include <QSizeF>
#include <QPointF>

#include <QDebug>

KateTextAnimation::KateTextAnimation(const KTextEditor::Range & range, KTextEditor::Attribute::Ptr attribute, KateViewInternal * view)
  : QObject(view)
  , m_range(range)
  , m_attribute(attribute)
  , m_doc(view->view()->doc())
  , m_view(view)
  , m_timeLine(new QTimeLine(250, this))
  , m_value(0.0)
{
  m_text = view->view()->doc()->text(range);

  connect(m_timeLine, SIGNAL(valueChanged(qreal)), this, SLOT(nextFrame(qreal)));
  connect(m_timeLine, SIGNAL(finished()), this, SLOT(deleteLater()));

  m_timeLine->setCurveShape(QTimeLine::SineCurve);
  m_timeLine->start();
}

KateTextAnimation::~KateTextAnimation()
{
  // if still running, we need to update the view a last time to avoid artifacts
  if (m_timeLine->state() == QTimeLine::Running) {
    m_timeLine->stop();
    nextFrame(0.0);
  }
}

QRectF KateTextAnimation::rectForText()
{
  const QFontMetricsF fm = m_view->view()->renderer()->currentFontMetrics();
  const int lineHeight = m_view->view()->renderer()->lineHeight();
  QPoint pixelPos = m_view->cursorToCoordinate(m_range.start(), /*bool realCursor*/ true, /*bool includeBorder*/ false);

  if (pixelPos.x() == -1 || pixelPos.y() == -1) {
    return QRectF();
  } else {
    QRectF rect(pixelPos.x(), pixelPos.y(),
                fm.width(m_view->view()->doc()->text(m_range)), lineHeight);
    const QPointF center = rect.center();
    const qreal factor = 1.0 + 0.5 * m_value;
    rect.setWidth(rect.width() * factor);
    rect.setHeight(rect.height() * factor);
    rect.moveCenter(center);
    return rect;
  }
}

void KateTextAnimation::draw(QPainter & painter)
{
  // could happen in corner cases: timeLine emitted finished(), but this object
  // is not yet deleted. Therefore, draw() might be called in paintEvent().
  if (m_timeLine->state() == QTimeLine::NotRunning) {
    return;
  }

  // get current rect and fill background
  QRectF rect = rectForText();
  painter.fillRect(rect, m_attribute->background());

  // scale font with animation
  QFont f = m_view->view()->renderer()->currentFont();
  f.setBold(m_attribute->fontBold());
  f.setPointSizeF(f.pointSizeF() * (1.0 + 0.5 * m_value));
  painter.setFont(f);

  painter.setPen(m_attribute->foreground().color());
  // finally draw contents on the view
  painter.drawText(rect, m_text);
}

void KateTextAnimation::nextFrame(qreal value)
{
  // cache previous rect for update
  const QRectF prevRect = rectForText();

  m_value = value;

  // next rect is used to draw the text
  const QRectF nextRect = rectForText();

  // due to rounding errors, increase the rect 1px to avoid artifacts
  const QRect updateRect = nextRect.united(prevRect).adjusted(-1, -1, 1, 1).toRect();

  // request repaint
  m_view->update(updateRect);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
