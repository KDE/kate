/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
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

#ifndef KATE_VI_REPLACE_MODE_INCLUDED
#define KATE_VI_REPLACE_MODE_INCLUDED

#include <QKeyEvent>
#include "katevimodebase.h"

class KateViMotion;
class KateView;
class KateViewInternal;

/**
 * Commands for the vi replace mode
 */

class KateViReplaceMode : public KateViModeBase
{
  public:
    KateViReplaceMode( KateViInputModeManager *viInputModeManager, KateView * view, KateViewInternal * viewInternal );
    ~KateViReplaceMode();

    bool handleKeypress( const QKeyEvent *e );

    bool commandInsertFromLine( int offset );

    bool commandMoveOneWordLeft();
    bool commandMoveOneWordRight();

    void overwrittenChar( const QChar &s ) { m_overwritten += s; }
    void backspace();
    void commandBackWord();
    void commandBackLine();

  private:
    QString m_overwritten;
};

#endif
