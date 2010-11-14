/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
 *  Copyright (C) 2007 Mirko Stocker <me@misto.ch>
 *  Copyright (C) 2008 David Nolden <david.nolden.kdevelop@art-master.de>
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

#include "katerenderrange.h"

#include <limits.h>

#include <kcolorutils.h>

void mergeAttributes(KTextEditor::Attribute::Ptr base, KTextEditor::Attribute::Ptr add)
{
  if(!add)
    return;

  bool hadBg = base->hasProperty(KTextEditor::Attribute::BackgroundBrush);
  bool hasBg = add->hasProperty(KTextEditor::Attribute::BackgroundBrush);

  bool hadFg = base->hasProperty(KTextEditor::Attribute::ForegroundBrush);
  bool hasFg = add->hasProperty(KTextEditor::Attribute::ForegroundBrush);

  if(((!hadBg || !hasBg) && (!hadFg || !hasFg))) {
    //Nothing to blend
    *base += *add;
    return;
  }

  //We eventually have to blend

  QBrush baseBgBrush, baseFgBrush;

  if(hadBg)
    baseBgBrush = base->background();

  if(hadFg)
    baseFgBrush = base->foreground();

  *base += *add;

  if(hadBg && hasBg)
  {
    QBrush bg = add->background();
    if(!bg.isOpaque()) {
      QColor mixWithColor = bg.color();
      mixWithColor.setAlpha(255);
      bg.setColor(KColorUtils::mix(baseBgBrush.color(), mixWithColor, bg.color().alphaF()));
      base->setBackground(bg);
    }
  }
  if(hadFg && hasFg)
  {
    QBrush fg = add->foreground();
    if(!fg.isOpaque()) {
      QColor mixWithColor = fg.color();
      mixWithColor.setAlpha(255);
      fg.setColor(KColorUtils::mix(baseFgBrush.color(), mixWithColor, fg.color().alphaF()));
      base->setForeground(fg);
    }
  }
}

bool KateRenderRange::isReady() const {
    return false;
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

void NormalRenderRange::addRange(KTextEditor::Range* range, KTextEditor::Attribute::Ptr attribute)
{
  m_ranges.append(pairRA(range, attribute));
}

KTextEditor::Cursor NormalRenderRange::nextBoundary() const
{
  return m_nextBoundary;
}

bool NormalRenderRange::advanceTo(const KTextEditor::Cursor& pos)
{
  int index = m_currentRange;
  while (index < m_ranges.count()) {
    const pairRA& p = m_ranges.at(index);
    KTextEditor::Range* r = p.first;
    if (r->end() <= pos) {
      ++index;
    } else {
      bool ret = index != m_currentRange;
      m_currentRange = index;

      if (r->start() > pos) {
        m_nextBoundary = r->start();
      } else {
        m_nextBoundary = r->end();
      }
      if (r->contains(pos)) {
        m_currentAttribute = p.second;
      } else {
        m_currentAttribute.clear();
      }

      return ret;
    }
  }

  m_nextBoundary = KTextEditor::Cursor(INT_MAX, INT_MAX);
  m_currentAttribute.clear();
  return false;
}

KTextEditor::Attribute::Ptr NormalRenderRange::currentAttribute() const
{
  return m_currentAttribute;
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

RenderRangeList::~RenderRangeList()
{
}

void RenderRangeList::advanceTo(const KTextEditor::Cursor& pos)
{
  foreach (KateRenderRange* r, *this)
    r->advanceTo(pos);

  //Delete lists that are ready, else the list may get too large due to temporaries
  for(int a = size()-1; a >= 0; --a) {
      KateRenderRange* r = at(a);
      if(r->isReady()) {
          delete r;
          removeAt(a);
      }
  }
}

bool RenderRangeList::hasAttribute() const
{
  foreach (KateRenderRange* r, *this)
    if (r->currentAttribute())
      return true;

  return false;
}

KTextEditor::Attribute::Ptr RenderRangeList::generateAttribute() const
{
  KTextEditor::Attribute::Ptr a;
  bool ownsAttribute = false;

  foreach (KateRenderRange* r, *this) {
    if (KTextEditor::Attribute::Ptr a2 = r->currentAttribute()) {
      if(!a) {
	a = a2;
      }else {
	if(!ownsAttribute) {
	  //Make an own copy of the attribute..
	  ownsAttribute = true;
	  a = new KTextEditor::Attribute(*a);
	}
        mergeAttributes(a, a2);
      }
    }
  }

  return a;
}

