/* This file is part of the KDE libraries
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>

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

#include "katerenderrange.h"

#include <limits.h>

#include "katesmartrange.h"
#include "katedynamicanimation.h"

SmartRenderRange::SmartRenderRange(KateSmartRange* range, bool useDynamic, KateView* view)
  : m_currentRange(0L)
  , m_view(view)
  , m_useDynamic(useDynamic)
{
  Q_ASSERT(range);
  if (range) {
    addTo(range);
    m_currentPos = range->start();
  }
}

KTextEditor::Cursor SmartRenderRange::nextBoundary() const
{
  if (!m_currentRange)
    return KTextEditor::Cursor(INT_MAX,INT_MAX);

  KTextEditor::SmartRange* r = m_currentRange->deepestRangeContaining(m_currentPos);
  foreach (KTextEditor::SmartRange* child, r->childRanges()) {
    if (child->start() > m_currentPos)
      return child->start();
  }
  return m_currentRange->end();
}

bool SmartRenderRange::advanceTo(const KTextEditor::Cursor& pos) const
{
  m_currentPos = pos;

  if (!m_currentRange)
    return false;

  bool ret = false;

  while (m_currentRange && !m_currentRange->contains(pos)) {
    m_attribs.pop();
    m_currentRange = m_currentRange->parentRange();
    ret = true;
  }

  if (!m_currentRange)
    return ret;

  KTextEditor::SmartRange* r = m_currentRange->deepestRangeContaining(pos);
  if (r != m_currentRange)
    ret = true;

  if (r)
    addTo(r);

  return ret;
}

KTextEditor::Attribute* SmartRenderRange::currentAttribute() const
{
  if (m_attribs.count())
    return const_cast<KTextEditor::Attribute*>(&m_attribs.top());
  return 0L;
}

void SmartRenderRange::addTo(KTextEditor::SmartRange* range) const
{
  KTextEditor::SmartRange* r = range;
  QStack<KTextEditor::SmartRange*> reverseStack;
  while (r != m_currentRange) {
    reverseStack.append(r);
    r = r->parentRange();
  }

  KTextEditor::Attribute a;
  if (m_attribs.count())
    a = m_attribs.top();

  while (reverseStack.count()) {
    KateSmartRange* r2 = static_cast<KateSmartRange*>(reverseStack.top());
    if (KTextEditor::Attribute* a2 = r2->attribute())
      a += *a2;

    if (m_useDynamic && r2->hasDynamic())
      foreach (KateDynamicAnimation* anim, r2->dynamicAnimations())
        anim->mergeToAttribute(a);

    m_attribs.append(a);
    reverseStack.pop();
  }

  m_currentRange = range;
}

NormalRenderRange::NormalRenderRange()
  : m_currentRange(0)
{
}

NormalRenderRange::~NormalRenderRange()
{
  QListIterator<pairRA> it = m_ranges;
  while (it.hasNext())
    delete it.next().first;
}

void NormalRenderRange::addRange(KTextEditor::Range* range, KTextEditor::Attribute* attribute)
{
  m_ranges.append(pairRA(range, attribute));
}

KTextEditor::Cursor NormalRenderRange::nextBoundary() const
{
  int index = m_currentRange;
  while (index < m_ranges.count()) {
    if (m_ranges.at(index).first->start() > m_currentPos)
      return m_ranges.at(index).first->start();

    else if (m_ranges.at(index).first->end() > m_currentPos)
      return m_ranges.at(index).first->end();

    ++index;

  }

  return KTextEditor::Cursor(INT_MAX, INT_MAX);
}

bool NormalRenderRange::advanceTo(const KTextEditor::Cursor& pos) const
{
  m_currentPos = pos;

  int index = m_currentRange;
  while (index < m_ranges.count()) {
    if (m_ranges.at(index).first->end() <= pos) {
      ++index;

    } else {
      bool ret = index != m_currentRange;
      m_currentRange = index;
      return ret;
    }
  }

  return false;
}

KTextEditor::Attribute* NormalRenderRange::currentAttribute() const
{
  if (m_currentRange < m_ranges.count() && m_ranges[m_currentRange].first->contains(m_currentPos))
    return m_ranges[m_currentRange].second;

  return 0L;
}

void RenderRangeList::appendRanges(const QList<KTextEditor::SmartRange*>& startingRanges, bool useDynamic, KateView* view)
{
  foreach (KTextEditor::SmartRange* range, startingRanges)
    append(new SmartRenderRange(static_cast<KateSmartRange*>(range), useDynamic, view));
}

KTextEditor::Cursor RenderRangeList::nextBoundary() const
{
  KTextEditor::Cursor ret = m_currentPos;
  bool first = true;
  foreach (KateRenderRange* r, *this) {
    if (first) {
      ret = r->nextBoundary();
      first = false;

    } else {
      KTextEditor::Cursor nb = r->nextBoundary();
      if (ret > nb)
        ret = nb;
    }
  }
  return ret;
}

bool RenderRangeList::advanceTo(const KTextEditor::Cursor& pos) const
{
  bool ret = false;

  foreach (KateRenderRange* r, *this)
    ret |= r->advanceTo(pos);

  return ret;
}

bool RenderRangeList::hasAttribute() const
{
  foreach (KateRenderRange* r, *this)
    if (r->currentAttribute())
      return true;

  return false;
}

KTextEditor::Attribute RenderRangeList::generateAttribute() const
{
  KTextEditor::Attribute a;

  foreach (KateRenderRange* r, *this)
    if (KTextEditor::Attribute* a2 = r->currentAttribute())
      a += *a2;

  return a;
}
