/* This file is part of the KDE libraries
   Copyright (C) 2003 Hamish Rodda <meddie@yoyo.its.monash.edu.au>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KATEARBITRARYHIGHLIGHT_H
#define KATEARBITRARYHIGHLIGHT_H

#include <qobject.h>
#include <qptrlist.h>
#include <qmap.h>
#include <qcolor.h>

#include "katesupercursor.h"

class KateDocument;
class KateView;

class ArbitraryHighlight
{
public:
  enum items {
    Weight = 0x1,
    Bold = 0x2,
    Italic = 0x4,
    Underline = 0x8,
    StrikeOut = 0x10,
    TextColor = 0x20,
    BGColor = 0x40
  };

  static ArbitraryHighlight merge(QPtrList<KateSuperRange> ranges);

  ArbitraryHighlight();

  QFont font(QFont ref);

  inline bool itemSet(int item) const
    { return item & m_itemsSet; };

  int itemsSet() const;

  int weight() const;
  void setWeight(int weight);

  void setBold(bool enable = true);

  bool italic() const;
  void setItalic(bool enable = true);

  bool underline() const;
  void setUnderline(bool enable = true);

  bool strikeOut() const;
  void setStrikeOut(bool enable = true);

  const QColor& textColor() const;
  void setTextColor(const QColor& color);

  const QColor& bgColor() const;
  void setBGColor(const QColor& color);

  friend bool operator ==(const ArbitraryHighlight& h1, const ArbitraryHighlight& h2);
  friend bool operator !=(const ArbitraryHighlight& h1, const ArbitraryHighlight& h2);

private:
  int m_weight;
  bool m_italic, m_underline, m_strikeout;
  QColor m_textColor, m_bgColor;
  int m_itemsSet;
};

class ArbitraryHighlightRange : public KateSuperRange, public ArbitraryHighlight
{
  Q_OBJECT

public:
  ArbitraryHighlightRange(KateSuperCursor* start, KateSuperCursor* end, QObject* parent = 0L, const char* name = 0L);
};

/**
 * An arbitrary highlighting interface for Kate.
 *
 * Ideas for more features:
 * - integration with syntax highlighting:
 *   - eg. a signal for when a new context is created, destroyed, changed
 *   - hopefully make this extension more complimentary to the current syntax highlighting
 */
class KateArbitraryHighlight : public QObject
{
  Q_OBJECT

public:
  KateArbitraryHighlight(KateDocument* parent = 0L, const char* name = 0L);

  void addHighlightToDocument(KateSuperRangeList* list);
  void addHighlightToView(KateSuperRangeList* list, KateView* view);

  KateSuperRangeList& rangesIncluding(uint line, KateView* view = 0L);

signals:
  void tagLines(KateView* view, KateSuperRange* range);

private slots:
  void slotRangeEliminated(KateSuperRange* range);

private:
  KateView* viewForRange(KateSuperRange* range);

  QMap<KateView*, QPtrList<KateSuperRangeList>* > m_viewHLs;
  QPtrList<KateSuperRangeList> m_docHLs;
};

#endif
