/* This file is part of the KDE libraries
   Copyright (C) 2003,2004,2005 Hamish Rodda <rodda@kde.org>

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

#include "katerangelist.h"

#include "katesuperrange.h"
#include "katedocument.h"

KateRangeList::KateRangeList(KateDocument* doc, QObject* parent, const char* name)
  : QObject(parent, name)
  , m_doc(doc)
  , m_topRange(new KateTopRange(doc, this))
  , m_rangeType(0L)
{
}

void KateRangeList::clear()
{
  topRange()->clearAllChildRanges();
}

/*void KateRangeList::connectAll()
{
  if (!m_connect) {
    m_connect = true;
    for (KateSuperRange* range = first(); range; range = next()) {
      if (dynamic_cast<KateSuperRange*>(range)) {
        connect(static_cast<KateSuperRange*>(range), SIGNAL(destroyed(QObject*)), SLOT(slotDeleted(QObject*)));
        connect(static_cast<KateSuperRange*>(range), SIGNAL(eliminated()), SLOT(slotEliminated()));
      }
    }
  }
}*/

/*void KateRangeList::slotDeleted(QObject* range)
{
  if (!count())
    emit listEmpty();
}*/

KateRangeList::operator KateDocument*() const
{
  return m_doc;
}

KateSuperRange* KateRangeList::findMostSpecificRange( const KTextEditor::Range & input ) const
{
  return m_topRange->findMostSpecificRange(input);
}

KateSuperRange* KateRangeList::firstRangeIncluding( const KTextEditor::Cursor & pos ) const
{
  return m_topRange->firstRangeIncluding(pos);
}

KateSuperRange* KateRangeList::deepestRangeIncluding( const KTextEditor::Cursor & pos ) const
{
  return m_topRange->deepestRangeIncluding(pos);
}

void KateRangeList::setRangeType( KateRangeType * t )
{
  m_rangeType = t;
  tagAll();
}

void KateRangeList::tagAll( ) const
{
  foreach (KateSuperRange* range, m_topRange->childRanges())
    // FIXME HACK HACK
    range->slotTagRange();
}

KateRangeList::~KateRangeList()
{
  if (rangeType())
    tagAll();
}

KateSuperRange * KateRangeList::topRange( ) const
{
  return m_topRange;
}

KateRangeType * KateRangeList::rangeType( ) const
{
  return m_rangeType;
}

/*int KateRangeList::compareItems(QPtrCollection::Item item1, QPtrCollection::Item item2)
{
  if (static_cast<KateSuperRange*>(item1)->start() == static_cast<KateSuperRange*>(item2)->start())
    if (static_cast<KateSuperRange*>(item1)->end() == static_cast<KateSuperRange*>(item2)->end())
      return 0;
  else
    return (static_cast<KateSuperRange*>(item1)->end() < static_cast<KateSuperRange*>(item2)->end()) ? -1 : 1;

  return (static_cast<KateSuperRange*>(item1)->start() < static_cast<KateSuperRange*>(item2)->start()) ? -1 : 1;
}

QPtrCollection::Item KateRangeList::newItem(QPtrCollection::Item d)
{
  KateSuperRange* simpleRange = static_cast<KateSuperRange*>(d);
  if (m_connect && dynamic_cast<KateSuperRange*>(simpleRange)) {
    connect(static_cast<KateSuperRange*>(simpleRange), SIGNAL(destroyed(QObject*)), SLOT(slotDeleted(QObject*)));
    connect(static_cast<KateSuperRange*>(simpleRange), SIGNAL(eliminated()), SLOT(slotEliminated()));
    connect(static_cast<KateSuperRange*>(simpleRange), SIGNAL(tagRange(KateSuperRange*)), SIGNAL(tagRange(KateSuperRange*)));

    // HACK HACK
    //static_cast<KateSuperRange*>(simpleRange)->slotTagRange();
  }

  if (m_trackingBoundaries) {
    m_columnBoundaries.append(&(simpleRange->start()));
    m_columnBoundaries.append(&(simpleRange->end()));
  }

  return QPtrList<KTextEditor::Range>::newItem(d);
}*/

#include "katerangelist.moc"
