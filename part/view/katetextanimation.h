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
#ifndef KATE_TEXT_ANIMATION_H
#define KATE_TEXT_ANIMATION_H

#include <QObject>
#include <QString>
#include <QRectF>

#include <ktexteditor/cursor.h>
#include <ktexteditor/range.h>
#include <ktexteditor/attribute.h>

class KateDocument;
class KateViewInternal;
class QTimeLine;
class QPainter;

/**
 * This class is used to flash text in the text view.
 * The duration of the flash animation is about 250 milliseconds.
 * When the animation is finished, it deletes itself.
 */
class KateTextAnimation : public QObject
{
  Q_OBJECT
public:
  KateTextAnimation(const KTextEditor::Range & range, KTextEditor::Attribute::Ptr attribute, KateViewInternal * view);
  virtual ~KateTextAnimation();

  // draw the text to highlight, given the current animation progress
  void draw(QPainter & painter);

public Q_SLOTS:
  // request repaint from view of the respective region
  void nextFrame(qreal value);

private:
  // calculate rect for the text to highlight, given the current animation progress
  QRectF rectForText();

private:
  KTextEditor::Range m_range;
  QString m_text;
  KTextEditor::Attribute::Ptr m_attribute;

  KateDocument * m_doc;
  KateViewInternal * m_view;
  QTimeLine * m_timeLine;
  qreal m_value;
};

#endif // KATE_TEXT_ANIMATION_H

// kate: space-indent on; indent-width 2; replace-tabs on;
