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

#include "katearbitraryhighlight.h"
#include "katearbitraryhighlight.moc"

#include "katesupercursor.h"
#include "katesuperrange.h"
#include "katedocument.h"
#include "katerangelist.h"

#include <kdebug.h>

KateArbitraryHighlight::KateArbitraryHighlight(KateDocument* parent, const char* name)
  : QObject(parent, name)
{
}

void KateArbitraryHighlight::addHighlightToDocument(KateRangeList* list)
{
  m_docHLs.append(list);
  connect(list, SIGNAL(rangeEliminated(KateSuperRange*)), SLOT(slotRangeEliminated(KateSuperRange*)));
  connect(list, SIGNAL(destroyed(QObject*)),SLOT(slotRangeListDeleted(QObject*)));
}

void KateArbitraryHighlight::addHighlightToView(KateRangeList* list, KateView* view)
{
  if (!m_viewHLs[view])
    m_viewHLs.insert(view, new QList<KateRangeList*>());

  m_viewHLs[view]->append(list);

  connect(list, SIGNAL(rangeEliminated(KateSuperRange*)), SLOT(slotTagRange(KateSuperRange*)));
  connect(list, SIGNAL(tagRange(KateSuperRange*)), SLOT(slotTagRange(KateSuperRange*)));
  connect(list, SIGNAL(destroyed(QObject*)),SLOT(slotRangeListDeleted(QObject*)));
}

void KateArbitraryHighlight::slotTagRange(KateSuperRange* range)
{
  emit tagLines(viewForRange(range), range);
}

KateView* KateArbitraryHighlight::viewForRange(KateSuperRange* range)
{
  if (!range->owningList())
    // Weird, should belong to a list if we're being asked...??
    return 0L;

  for (QMap<KateView*, QList<KateRangeList*>* >::Iterator it = m_viewHLs.begin(); it != m_viewHLs.end(); it++)
    for (QList<KateRangeList*>::ConstIterator it2 = (*it)->constBegin(); it2 != (*it)->constEnd(); ++it2)
      if (range->owningList() == *it2)
        return it.key();

  // This must belong to a document-global highlight
  return 0L;
}

QList< KateSuperRange * > KateArbitraryHighlight::startingRanges( const KTextEditor::Cursor & pos, KateView * view ) const
{
  QList<KateSuperRange*> ret;

  foreach (KateRangeList* list, m_docHLs)
    if (KateSuperRange* r = list->deepestRangeIncluding(pos))
      ret.append(r);

  if (view && m_viewHLs.contains(view))
    for (QList<KateRangeList*>::ConstIterator it = m_viewHLs[view]->constBegin(); it != m_viewHLs[view]->constEnd(); ++it)
      if (KateSuperRange* r = (*it)->deepestRangeIncluding(pos))
        ret.append(r);

  return ret;
}

// kate: space-indent on; indent-width 2; replace-tabs on;

