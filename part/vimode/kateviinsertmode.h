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

#ifndef KATE_VI_INSERT_MODE_INCLUDED
#define KATE_VI_INSERT_MODE_INCLUDED

#include <QKeyEvent>

class KateViMotion;
class KateView;
class KateViewInternal;

/**
 * Commands for the vi insert mode
 */

class KateViInsertMode
{
  public:
    KateViInsertMode( KateView * view, KateViewInternal * viewInternal );
    ~KateViInsertMode();

    bool handleKeypress( const QKeyEvent *e );

    bool commandInsertFromAbove();
    bool commandInsertFromBelow();

    bool commandDeleteWord();

    bool commandIndent();
    bool commandUnindent();

    bool commandToFirstCharacterInFile();
    bool commandToLastCharacterInFile();

    bool commandMoveOneWordLeft();
    bool commandMoveOneWordRight();

  private:
    QChar getCharAtVirtualColumn( QString &line, int virtualColumn, int tabWidht ) const;
    KateView *m_view;
    KateViewInternal *m_viewInternal;
};

#endif
