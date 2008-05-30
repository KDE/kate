/* This file is part of the KDE libraries
 * Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "katevinormalmodecommand.h"

KateViNormalModeCommand::KateViNormalModeCommand( KateViNormalMode *parent, QString pattern, bool(KateViNormalMode::*pt2Func)(), bool regex )
{
  m_parent = parent;
  m_pattern = pattern;
  m_regex = regex;
  m_pt2Func = pt2Func;
}

KateViNormalModeCommand::~KateViNormalModeCommand()
{
}

void KateViNormalModeCommand::execute()
{
  (m_parent->*m_pt2Func)();
}

bool KateViNormalModeCommand::matches( QString pattern )
{
  if ( !m_regex )
    return m_pattern.startsWith( pattern );
  else {
    QRegExp re( m_pattern );
    re.exactMatch( pattern );
    return ( re.matchedLength() == pattern.length() );
  }
}

bool KateViNormalModeCommand::matchesExact( QString pattern )
{
  if ( !m_regex )
    return ( m_pattern == pattern );
  else {
    QRegExp re( m_pattern );
    return re.exactMatch( pattern );
  }
}
