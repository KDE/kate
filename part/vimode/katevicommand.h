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

#ifndef KATE_VI_COMMAND_H
#define KATE_VI_COMMAND_H

class KateViNormalMode;

enum KateViCommandFlags {
    REGEX_PATTERN = 0x1,    // the pattern is a regex
    NEEDS_MOTION = 0x2,     // the command needs a motion before it can be executed
    SHOULD_NOT_RESET = 0x4, // the command should not cause the current mode to be left
    IS_CHANGE = 0x8         // the command changes the buffer
};

class KateViCommand {
  public:
    KateViCommand( KateViNormalMode *parent, QString pattern,
        bool ( KateViNormalMode::*pt2Func)(), unsigned int flags = 0 );
    ~KateViCommand();

    bool matches( QString pattern ) const;
    bool matchesExact( QString pattern ) const;
    bool execute() const;
    const QString pattern() const { return m_pattern; }
    bool isRegexPattern() const { return m_flags & REGEX_PATTERN; }
    bool needsMotion() const { return m_flags & NEEDS_MOTION; }
    bool shouldReset() const { return !( m_flags & SHOULD_NOT_RESET ); }
    bool isChange() const { return m_flags & IS_CHANGE; }

  protected:
    KateViNormalMode *m_parent;
    QString m_pattern;
    unsigned int m_flags;
    bool ( KateViNormalMode::*m_ptr2commandMethod)();
    KateViKeySequenceParser *m_keyParser;
};

#endif
