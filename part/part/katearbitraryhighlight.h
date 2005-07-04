/* This file is part of the KDE libraries
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATEARBITRARYHIGHLIGHT_H
#define KATEARBITRARYHIGHLIGHT_H

#include "kateattribute.h"
#include "katesupercursor.h"

#include <qobject.h>
#include <qptrlist.h>
#include <qmap.h>

class KateDocument;
class KateView;

class KateArbitraryHighlightRange : public KateSuperRange, public KateAttribute
{
  Q_OBJECT

public:
  KateArbitraryHighlightRange(KateSuperCursor* start, KateSuperCursor* end, QObject* parent = 0L, const char* name = 0L);
  KateArbitraryHighlightRange(KateDocument* doc, const KateRange& range, QObject* parent = 0L, const char* name = 0L);
  KateArbitraryHighlightRange(KateDocument* doc, const KateTextCursor& start, const KateTextCursor& end, QObject* parent = 0L, const char* name = 0L);

	virtual ~KateArbitraryHighlightRange();

  virtual void changed() { slotTagRange(); };

  static KateAttribute merge(QPtrList<KateSuperRange> ranges);
};

/**
 * An arbitrary highlighting interface for Kate.
 *
 * Ideas for more features:
 * - integration with syntax highlighting:
 *   - eg. a signal for when a new context is created, destroyed, changed
 *   - hopefully make this extension more complimentary to the current syntax highlighting
 * - signal for cursor movement
 * - signal for mouse movement
 * - identical highlight for whole list
 * - signals for view movement
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
  void slotTagRange(KateSuperRange* range);
  void slotRangeListDeleted(QObject* obj);
private:
  KateView* viewForRange(KateSuperRange* range);

  QMap<KateView*, QPtrList<KateSuperRangeList>* > m_viewHLs;
  QPtrList<KateSuperRangeList> m_docHLs;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
