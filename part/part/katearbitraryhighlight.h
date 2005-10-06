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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATEARBITRARYHIGHLIGHT_H
#define KATEARBITRARYHIGHLIGHT_H

#include <ktexteditor/attribute.h>
#include "katesmartcursor.h"

#include <qobject.h>
#include <q3ptrlist.h>
#include <qmap.h>

class KateDocument;
class KateView;
class KTextEditor::Attribute;
class KateRangeList;
namespace KTextEditor { class Range; }
class KateSmartRange;

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

  void addHighlightToDocument(KateRangeList* list);
  void addHighlightToView(KateRangeList* list, KateView* view);

  QList<KateSmartRange*> startingRanges(const KTextEditor::Cursor& pos, KateView* view = 0L) const;
  
signals:
  void tagLines(KateView* view, KTextEditor::Range* range);

private slots:
  void slotTagRange(KateSmartRange* range);
  // FIXME implement or redesign??
  //void slotRangeListDeleted(QObject* obj);
  
private:
  KateView* viewForRange(KateSmartRange* range);

  QMap<KateView*, QList<KateRangeList*>* > m_viewHLs;
  QList<KateRangeList*> m_docHLs;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
