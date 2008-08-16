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
#include "katevimotion.h"
#include "katevirange.h"

#include <QKeyEvent>
#include <QVector>
#include <QStack>
#include <ktexteditor/cursor.h>
#include "katevikeysequenceparser.h"

class KateViMotion;

/**
 * Commands for the vi normal mode
 */

class KateViNormalMode : public QObject {
  friend class KateViInsertMode;

  public:
    KateViNormalMode( KateView * view, KateViewInternal * viewInternal );
    ~KateViNormalMode();

    bool handleKeypress( QKeyEvent *e );

    bool commandEnterInsertMode();
    bool commandEnterInsertModeAppend();
    bool commandEnterInsertModeAppendEOL();

    bool commandEnterVisualMode();
    bool commandEnterVisualLineMode();

    bool commandDelete();
    bool commandDeleteToEOL();
    bool commandDeleteLine();

    bool commandMakeLowercase();
    bool commandMakeLowercaseLine();
    bool commandMakeUppercase();
    bool commandMakeUppercaseLine();

    bool commandOpenNewLineUnder();
    bool commandOpenNewLineOver();

    bool commandJoinLines();

    bool commandChange();
    bool commandChangeLine();
    bool commandChangeToEOL();

    bool commandYank();
    bool commandYankLine();
    bool commandYankToEOL();

    bool commandFormatLines();

    bool commandAddIndentation();
    bool commandRemoveIndentation();

    bool commandPaste();
    bool commandPasteBefore();

    bool commandDeleteChar();
    bool commandDeleteCharBackward();

    bool commandReplaceCharacter();

    bool commandSwitchToCmdLine();
    bool commandSearch();
    bool commandUndo();
    bool commandRedo();

    bool commandSetMark();

    bool commandFindNext();
    bool commandFindPrev();

    bool commandIndentLine();
    bool commandUnindentLine();
    bool commandIndentLines();
    bool commandUnindentLines();

    bool commandScrollPageDown();
    bool commandScrollPageUp();

    // MOTIONS

    KateViRange motionLeft();
    KateViRange motionRight();
    KateViRange motionDown();
    KateViRange motionUp();

    KateViRange motionWordForward();
    KateViRange motionWordBackward();
    KateViRange motionWORDForward();
    KateViRange motionWORDBackward();

    KateViRange motionToEndOfWord();
    KateViRange motionToEndOfWORD();

    KateViRange motionFindChar();
    KateViRange motionFindCharBackward();
    KateViRange motionToChar();
    KateViRange motionToCharBackward();

    KateViRange motionToEOL();
    KateViRange motionToColumn0();
    KateViRange motionToFirstCharacterOfLine();

    KateViRange motionToLineFirst();
    KateViRange motionToLineLast();

    KateViRange motionToScreenColumn();
    KateViRange motionToMatchingBracket();

    KateViRange motionToMark();
    KateViRange motionToMarkLine();

    // TEXT OBJECTS

    KateViRange textObjectAWord();
    KateViRange textObjectInnerWord();
    KateViRange textObjectAWORD();
    KateViRange textObjectInnerWORD();

    KateViRange textObjectAQuoteDouble();
    KateViRange textObjectInnerQuoteDouble();

    KateViRange textObjectAQuoteSingle();
    KateViRange textObjectInnerQuoteSingle();

    KateViRange textObjectAParen();
    KateViRange textObjectInnerParen();

    KateViRange textObjectABracket();
    KateViRange textObjectInnerBracket();

    unsigned int getCount() const { return ( m_count > 0 ) ? m_count : 1; }
    QChar getChosenRegister( const QChar &defaultReg ) const;
    QString getRegisterContent( const QChar &reg ) const;
    void fillRegister( const QChar &reg, const QString &text);
    void addCurrentPositionToJumpList();

    void error( const QString &errorMsg ) const;

  protected:
    bool deleteRange( KateViRange &r, bool linewise = true );
    const QString getRange( KateViRange &r, bool linewise = true ) const;
    void reset();
    virtual void abort();
    void initializeCommands();
    QString getLine( int lineNumber = -1 ) const;
    KTextEditor::Cursor findNextWordStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    KTextEditor::Cursor findNextWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    KTextEditor::Cursor findPrevWordStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    KTextEditor::Cursor findPrevWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    KTextEditor::Cursor findWordEnd( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    KTextEditor::Cursor findWORDEnd( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    KateViRange findSurrounding( const QChar &c1, const QChar &c2, bool inner = false );
    virtual void goToPos( KateViRange r );

    KateView *m_view;
    KateViewInternal *m_viewInternal;
    QString m_keys;
    QString m_keysVerbatim;
    unsigned int m_count;
    unsigned int m_countTemp;
    QChar m_register;
    bool m_findWaitingForChar;
    int m_waitingForMotionOrTextObject;

    QString m_extraWordCharacters;

    QVector<KateViNormalModeCommand *> m_commands;
    QVector<KateViMotion *> m_motions;
    QVector<int> m_matchingCommands;
    QVector<int> m_matchingMotions;
    QStack<int> m_awaitingMotionOrTextObject;

    int m_stickyColumn;
    int m_motionOperatorIndex;

    KateViRange m_commandRange;

    // registers
    QChar m_defaultRegister;
    QString m_registerTemp;

    // marks
    QMap<QChar, KTextEditor::SmartCursor*> *m_marks;

    KateViKeySequenceParser *m_keyParser;
};

#endif
