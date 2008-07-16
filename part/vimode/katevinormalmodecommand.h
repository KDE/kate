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

#include "katevinormalmode.h"
#include "katevikeysequenceparser.h"

#ifndef VI_NORMAL_MODE_COMMAND_H
#define VI_NORMAL_MODE_COMMAND_H

class KateViNormalMode;

class KateViNormalModeCommand {
  public:
    KateViNormalModeCommand( KateViNormalMode *parent, QString pattern,
        bool ( KateViNormalMode::*pt2Func)(), bool regex = true );
    KateViNormalModeCommand( KateViNormalMode *parent, QString pattern,
        bool ( KateViNormalMode::*pt2Func)(), bool regex, bool needsMotionOrTextObject );
    ~KateViNormalModeCommand();

    bool matches( QString pattern ) const;
    bool matchesExact( QString pattern ) const;
    bool execute() const;
    bool needsMotionOrTextObject() { return m_needsMotionOrTextObject; }
    QString pattern() { return m_pattern; }

  protected:
    KateViNormalMode *m_parent;
    QString m_pattern;
    bool m_needsMotionOrTextObject;
    bool m_regex;
    bool ( KateViNormalMode::*m_ptr2commandMethod)();
    KateViKeySequenceParser *m_keyParser;
};

#endif
