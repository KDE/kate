/* This file is part of the KDE libraries
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#include "katesmartregion.h"

#include "katedocument.h"
#include "katesmartmanager.h"

KateSmartRegion::KateSmartRegion(KateDocument* document)
  : m_bounding(document->smartManager()->newSmartRange(KTextEditor::Range::invalid()))
{
}

void KateSmartRegion::addRange( KateSmartRange * range )
{
  m_source.append(range);

  if (m_bounding->isValid()) {
    if (range->contains(*m_bounding)) {
      *m_bounding = *range;

    } else {
      if (range->start() < m_bounding->start())
        m_bounding->start() = range->start();
      if (range->end() > m_bounding->end())
        m_bounding->end() = range->end();
    }

  } else {
    *m_bounding = *range;
  }
}

void KateSmartRegion::removeRange( KateSmartRange * range )
{
  m_source.remove(range);

  if (m_source.count()) {
    KTextEditor::Range process = KTextEditor::Range::invalid();
  } else {
    *m_bounding = KTextEditor::Range::invalid();
  }
}


