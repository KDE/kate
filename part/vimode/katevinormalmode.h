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

#ifndef KATE_VI_NORMAL_MODE_INCLUDED
#define KATE_VI_NORMAL_MODE_INCLUDED

#include "kateview.h"
#include "kateviewinternal.h"
#include "katevinormalmodecommand.h"

#include <QKeyEvent>
#include <ktexteditor/cursor.h>

/**
 * Commands for the vi normal mode
 */

class KateViNormalMode {
  public:
    KateViNormalMode( KateView * view, KateViewInternal * viewInternal );
    ~KateViNormalMode();

    bool handleKeypress( QKeyEvent *e );

    bool commandEnterInsertMode();
    bool commandEnterInsertModeAppend();
    bool commandEnterInsertModeAppendEOL();

    bool commandGoLeft();
    bool commandGoRight();
    bool commandGoDown();
    bool commandGoUp();
    bool commandGoDownDisplayLine();
    bool commandGoUpDisplayLine();

    bool commandFindChar();
    bool commandFindCharBackward();
    bool commandToChar();
    bool commandToCharBackward();

    bool commandGoToFirstCharacterOfLine();
    bool commandGoToEOL();

    bool commandGoWordForward();
    bool commandGoWordBackward();
    bool commandGoWORDForward();
    bool commandGoWORDBackward();

    unsigned int getCount() const { return ( m_count > 0 ) ? m_count : 1; }

  private:
    void reset();
    void initializeCommands();
    QString getLine( int lineNumber = -1 ) const;
    void goToStickyColumn( KTextEditor::Cursor &c );

    KateView *m_view;
    KateViewInternal *m_viewInternal;
    QString m_keys;
    unsigned int m_count;
    bool m_gettingCount;
    bool m_findWaitingForChar;

    QVector<KateViNormalModeCommand *> m_commands;
    QVector<int> m_matches;

    int m_stickyColumn;
};

#endif
