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

#include "katearbitraryhighlight.h"

#include <qfont.h>

#include <kdebug.h>

#include "katesupercursor.h"
#include "katedocument.h"

ArbitraryHighlightRange::ArbitraryHighlightRange(KateSuperCursor* start, KateSuperCursor* end, QObject* parent, const char* name)
  : KateSuperRange(start, end, parent, name)
{
}

ArbitraryHighlight::ArbitraryHighlight()
  : m_weight(QFont::Normal)
  , m_italic(false)
  , m_underline(false)
  , m_strikeout(false)
  , m_itemsSet(0)
{
}

ArbitraryHighlight ArbitraryHighlight::merge(QPtrList<KateSuperRange> ranges)
{
  ranges.sort();

  ArbitraryHighlight ret;

  if (ranges.first() && ranges.current()->inherits("ArbitraryHighlightRange"))
    ret = *(static_cast<ArbitraryHighlightRange*>(ranges.current()));

  KateSuperRange* r;
  while ((r = ranges.next())) {
    if (r->inherits("ArbitraryHighlightRange")) {
      ArbitraryHighlightRange* hl = static_cast<ArbitraryHighlightRange*>(r);
      if (hl->itemSet(Weight))
        ret.setWeight(hl->weight());

      if (hl->itemSet(Italic))
        ret.setItalic(hl->italic());

      if (hl->itemSet(Underline))
        ret.setUnderline(hl->underline());

      if (hl->itemSet(StrikeOut))
        ret.setStrikeOut(hl->strikeOut());

      if (hl->itemSet(TextColor))
        ret.setTextColor(hl->textColor());

      if (hl->itemSet(BGColor))
        ret.setBGColor(hl->bgColor());
    }
  }

  return ret;
}

QFont ArbitraryHighlight::font(QFont ref)
{
  if (itemSet(Weight))
    ref.setWeight(weight());
  if (itemSet(Italic))
    ref.setItalic(italic());
  if (itemSet(Underline))
    ref.setUnderline(underline());
  if (itemSet(StrikeOut))
    ref.setStrikeOut(strikeOut());

  return ref;
}

int ArbitraryHighlight::itemsSet() const
{
  return m_itemsSet;
}

int ArbitraryHighlight::weight() const
{
  return m_weight;
}

void ArbitraryHighlight::setWeight(int weight)
{
  m_itemsSet |= Weight;

  m_weight = weight;
}

void ArbitraryHighlight::setBold(bool enable)
{
  setWeight(enable ? QFont::Bold : QFont::Normal);
}

bool ArbitraryHighlight::italic() const
{
  return m_italic;
}

void ArbitraryHighlight::setItalic(bool enable)
{
  m_itemsSet |= Italic;

  m_italic = enable;
}

bool ArbitraryHighlight::underline() const
{
  return m_underline;
}

void ArbitraryHighlight::setUnderline(bool enable)
{
  m_itemsSet |= Underline;

  m_underline = enable;
}

bool ArbitraryHighlight::strikeOut() const
{
  return m_strikeout;
}

void ArbitraryHighlight::setStrikeOut(bool enable)
{
  m_itemsSet |= StrikeOut;

  m_strikeout = enable;
}

const QColor& ArbitraryHighlight::textColor() const
{
  return m_textColor;
}

void ArbitraryHighlight::setTextColor(const QColor& color)
{
  m_itemsSet |= TextColor;

  m_textColor = color;
}

const QColor& ArbitraryHighlight::bgColor() const
{
  return m_bgColor;
}

void ArbitraryHighlight::setBGColor(const QColor& color)
{
  m_itemsSet |= BGColor;

  m_bgColor = color;
}

bool operator ==(const ArbitraryHighlight& h1, const ArbitraryHighlight& h2)
{
  if (h1.m_itemsSet != h2.m_itemsSet)
    return false;

  if (h1.itemSet(ArbitraryHighlight::Weight))
    if (h1.m_weight != h2.m_weight)
      return false;

  if (h1.itemSet(ArbitraryHighlight::Italic))
    if (h1.m_italic != h2.m_italic)
      return false;

  if (h1.itemSet(ArbitraryHighlight::Underline))
    if (h1.m_underline != h2.m_underline)
      return false;

  if (h1.itemSet(ArbitraryHighlight::StrikeOut))
    if (h1.m_strikeout != h2.m_strikeout)
      return false;

  if (h1.itemSet(ArbitraryHighlight::TextColor))
    if (h1.m_textColor != h2.m_textColor)
      return false;

  if (h1.itemSet(ArbitraryHighlight::BGColor))
    if (h1.m_bgColor != h2.m_bgColor)
      return false;

  return true;
}

bool operator !=(const ArbitraryHighlight& h1, const ArbitraryHighlight& h2)
{
  return !(h1 == h2);
}

KateArbitraryHighlight::KateArbitraryHighlight(KateDocument* parent, const char* name)
  : QObject(parent, name)
{
}

void KateArbitraryHighlight::addHighlightToDocument(KateSuperRangeList* list)
{
  m_docHLs.append(list);
  connect(list, SIGNAL(rangeEliminated(KateSuperRange*)), SLOT(slotRangeEliminated(KateSuperRange*)));
}

void KateArbitraryHighlight::addHighlightToView(KateSuperRangeList* list, KateView* view)
{
  if (!m_viewHLs[view])
    m_viewHLs.insert(view, new QPtrList<KateSuperRangeList>());

  m_viewHLs[view]->append(list);

  connect(list, SIGNAL(rangeEliminated(KateSuperRange*)), SLOT(slotRangeEliminated(KateSuperRange*)));
}

KateSuperRangeList& KateArbitraryHighlight::rangesIncluding(uint line, KateView* view)
{
  // OPTIMISE make return value persistent

  static KateSuperRangeList s_return(false);
  Q_ASSERT(!s_return.autoDelete());
  s_return.clear();

  //--- TEMPORARY OPTIMISATION: return the actual range when there are none or one. ---
  if (m_docHLs.count() + m_viewHLs.count() == 0)
    return s_return;
  else if (m_docHLs.count() + m_viewHLs.count() == 1)
    if (m_docHLs.count())
      return *(m_docHLs.first());
    else
      if (m_viewHLs.values().first() && m_viewHLs.values().first()->count() == 1)
        if (m_viewHLs.keys().first() == view && m_viewHLs.values().first())
          return *(m_viewHLs.values().first()->first());
  //--- END Temporary optimisation ---

  if (view) {
    QPtrList<KateSuperRangeList>* list = m_viewHLs[view];
    if (list)
      for (KateSuperRangeList* l = list->first(); l; l = list->next())
        if (l->count())
          s_return.appendList(l->rangesIncluding(line));

  } else {
    for (QMap<KateView*, QPtrList<KateSuperRangeList>* >::Iterator it = m_viewHLs.begin(); it != m_viewHLs.end(); it++)
      for (KateSuperRangeList* l = (*it)->first(); l; l = (*it)->next())
        if (l->count())
          s_return.appendList(l->rangesIncluding(line));
  }

  for (KateSuperRangeList* l = m_docHLs.first(); l; l = m_docHLs.next())
    if (l->count())
      s_return.appendList(l->rangesIncluding(line));

  return s_return;
}

void KateArbitraryHighlight::slotRangeEliminated(KateSuperRange* range)
{
  emit tagLines(viewForRange(range), range);
}

KateView* KateArbitraryHighlight::viewForRange(KateSuperRange* range)
{
  for (QMap<KateView*, QPtrList<KateSuperRangeList>* >::Iterator it = m_viewHLs.begin(); it != m_viewHLs.end(); it++)
    for (KateSuperRangeList* l = (*it)->first(); l; l = (*it)->next())
      if (l->contains(range))
        return it.key();

  // This must belong to a document-global highlight
  return 0L;
}

#include "katearbitraryhighlight.moc"
