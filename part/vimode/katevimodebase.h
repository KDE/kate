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

#ifndef KATE_VI_MODE_BASE_INCLUDED
#define KATE_VI_MODE_BASE_INCLUDED

#include <ktexteditor/cursor.h>
#include "katevirange.h"

#include <QList>

class QKeyEvent;
class QString;
class QRegExp;
class KateView;
class KateViewInternal;
class KateViVisualMode;
class KateViNormalMode;
class KateViInputModeManager;

class KateViModeBase
{
  public:
    KateViModeBase() {};
    virtual ~KateViModeBase() {};

    /**
     * @return normal mode command accumulated so far
     */
    QString getVerbatimKeys() const;

  protected:
    // helper methods
    bool deleteRange( KateViRange &r, bool linewise = true, bool addToRegister = true );
    const QString getRange( KateViRange &r, bool linewise = true ) const;
    const QString getLine( int lineNumber = -1 ) const;
    const QChar getCharUnderCursor() const;
    KTextEditor::Cursor findNextWordStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    KTextEditor::Cursor findNextWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    KTextEditor::Cursor findPrevWordStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    KTextEditor::Cursor findPrevWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    KTextEditor::Cursor findWordEnd( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    KTextEditor::Cursor findWORDEnd( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    KateViRange findSurrounding( const QChar &c1, const QChar &c2, bool inner = false );
    int findLineStartingWitchChar( const QChar &c, unsigned int count, bool forward = true ) const;
    void updateCursor( const KTextEditor::Cursor &c ) const;

    unsigned int getCount() const { return ( m_count > 0 ) ? m_count : 1; }

    bool startNormalMode();
    bool startInsertMode();
    bool startVisualMode();
    bool startVisualLineMode();

    void error( const QString &errorMsg ) const;
    void message( const QString &msg ) const;

    QChar getChosenRegister( const QChar &defaultReg ) const;
    QString getRegisterContent( const QChar &reg ) const;
    void fillRegister( const QChar &reg, const QString &text);

    QChar m_register;

    KateViRange m_commandRange;
    unsigned int m_count;

    QString m_extraWordCharacters;
    QString m_keysVerbatim;

    KateView *m_view;
    KateViewInternal *m_viewInternal;
    KateViInputModeManager* m_viInputModeManager;
};

#endif
