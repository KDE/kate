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
#include "katevicommand.h"
#include "katevimotion.h"
#include "katevirange.h"
#include "katevimodebase.h"

#include <QKeyEvent>
#include <QVector>
#include <QStack>
#include <QHash>
#include <QRegExp>
#include <ktexteditor/cursor.h>
#include "katevikeysequenceparser.h"

class KateViMotion;
class KateViInputModeManager;

/**
 * Commands for the vi normal mode
 */

class KateViNormalMode : public KateViModeBase
{
  public:
    KateViNormalMode( KateViInputModeManager *viInputModeManager, KateView * view, KateViewInternal * viewInternal );
    virtual ~KateViNormalMode();

    bool handleKeypress( QKeyEvent *e );

    bool commandEnterInsertMode();
    bool commandEnterInsertModeAppend();
    bool commandEnterInsertModeAppendEOL();

    bool commandEnterVisualMode();
    bool commandEnterVisualLineMode();
    bool commandToOtherEnd();

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
    bool commandSubstituteChar();
    bool commandSubstituteLine();

    bool commandYank();
    bool commandYankLine();
    bool commandYankToEOL();

    bool commandFormatLines();

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

    bool commandAbort();

    bool commandPrintCharacterCode();

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

    KateViRange motionToMark();
    KateViRange motionToMarkLine();

    KateViRange motionToMatchingItem();

    KateViRange motionToPreviousBraceBlockStart();
    KateViRange motionToNextBraceBlockStart();
    KateViRange motionToPreviousBraceBlockEnd();
    KateViRange motionToNextBraceBlockEnd();

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

    void addCurrentPositionToJumpList();

  protected:
    void resetParser();
    virtual void reset();
    void initializeCommands();
    QRegExp generateMatchingItemRegex();
    virtual void goToPos( KateViRange r );
    void executeCommand( const KateViCommand* cmd );

    QString m_keys;
    unsigned int m_countTemp;
    bool m_findWaitingForChar;
    int m_waitingForMotionOrTextObject;

    QVector<KateViCommand *> m_commands;
    QVector<KateViMotion *> m_motions;
    QVector<int> m_matchingCommands;
    QVector<int> m_matchingMotions;
    QStack<int> m_awaitingMotionOrTextObject;

    int m_stickyColumn;
    int m_motionOperatorIndex;

    // registers
    QChar m_defaultRegister;
    QString m_registerTemp;

    // marks
    QMap<QChar, KTextEditor::SmartCursor*> *m_marks;

    // item matching ('%' motion)
    QHash<QString, QString> m_matchingItems;
    QRegExp m_matchItemRegex;

    KateViKeySequenceParser *m_keyParser;
};

#endif
