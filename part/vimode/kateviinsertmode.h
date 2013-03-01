/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008-2011 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
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

#ifndef KATE_VI_INSERT_MODE_INCLUDED
#define KATE_VI_INSERT_MODE_INCLUDED

#include <QKeyEvent>
#include "katevimodebase.h"
#include "katepartprivate_export.h"

class KateViMotion;
class KateView;
class KateViewInternal;

/**
 * Commands for the vi insert mode
 */

enum BlockInsert {
    None,
    Prepend,
    Append,
    AppendEOL
};

class KATEPART_TESTS_EXPORT KateViInsertMode : public KateViModeBase
{
  public:
    KateViInsertMode( KateViInputModeManager *viInputModeManager, KateView * view, KateViewInternal * viewInternal );
    ~KateViInsertMode();

    bool handleKeypress( const QKeyEvent *e );

    bool commandInsertFromAbove();
    bool commandInsertFromBelow();

    bool commandDeleteWord();
    bool commandNewLine();
    bool commandDeleteCharBackward();

    bool commandIndent();
    bool commandUnindent();

    bool commandToFirstCharacterInFile();
    bool commandToLastCharacterInFile();

    bool commandMoveOneWordLeft();
    bool commandMoveOneWordRight();

    bool commandCompleteNext();
    bool commandCompletePrevious();

    bool commandInsertContentOfRegister();
    bool commandSwitchToNormalModeForJustOneCommand();

    // mappings not supported in insert mode yet
    void addMapping( const QString &from, const QString &to ) { Q_UNUSED(from) Q_UNUSED(to) }
    const QString getMapping( const QString &from ) const { Q_UNUSED(from) return QString(); }
    const QStringList getMappings() const { return QStringList(); }

    void setBlockPrependMode( KateViRange blockRange );
    void setBlockAppendMode( KateViRange blockRange, BlockInsert b );

    void setCount(int count) { m_count = count;};
    void setCountedRepeatsBeginOnNewLine(bool countedRepeatsBeginOnNewLine) { m_countedRepeatsBeginOnNewLine = countedRepeatsBeginOnNewLine;};

  protected:
    BlockInsert m_blockInsert;
    unsigned int m_eolPos; // length of first line in eol mode before text is appended
    KateViRange m_blockRange;

    QString m_registerTemp;
    QString m_keys;

    unsigned int m_count;
    bool m_countedRepeatsBeginOnNewLine;

    void leaveInsertMode( bool force = false);
};

#endif
