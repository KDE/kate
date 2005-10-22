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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "rangefeedback.h"

using namespace KTextEditor;


SmartRangeWatcher::~ SmartRangeWatcher( )
{
}

void SmartRangeWatcher::positionChanged( SmartRange * )
{
}

void SmartRangeWatcher::contentsChanged( SmartRange * )
{
}

void SmartRangeWatcher::eliminated( SmartRange * )
{
}

void SmartRangeWatcher::contentsChanged( SmartRange * , SmartRange *  )
{
}

SmartRangeNotifier::SmartRangeNotifier( )
  : m_wantDirectChanges(true)
{
}

bool SmartRangeNotifier::wantsDirectChanges( ) const
{
  return m_wantDirectChanges;
}

void SmartRangeNotifier::setWantsDirectChanges( bool wantsDirectChanges )
{
  m_wantDirectChanges = wantsDirectChanges;
}

SmartRangeWatcher::SmartRangeWatcher( )
  : m_wantDirectChanges(true)
{
}

bool SmartRangeWatcher::wantsDirectChanges( ) const
{
  return m_wantDirectChanges;
}

void SmartRangeWatcher::setWantsDirectChanges( bool wantsDirectChanges )
{
  m_wantDirectChanges = wantsDirectChanges;
}

void SmartRangeWatcher::deleted( SmartRange * )
{
}

#include "rangefeedback.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
