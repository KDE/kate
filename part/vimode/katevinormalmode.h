/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 - 2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2009 Paul Gideon Dann <pdgiddie@gmail.com>
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
#include "katevikeyparser.h"
#include "katepartprivate_export.h"

class KateViMotion;
class KateViInputModeManager;

/**
 * Commands for the vi normal mode
 */
class KATEPART_TESTS_EXPORT KateViNormalMode : public KateViModeBase
{
  Q_OBJECT

  public slots:
    void mappingTimerTimeOut();

  public:
    KateViNormalMode( KateViInputModeManager *viInputModeManager, KateView * view, KateViewInternal * viewInternal );
    virtual ~KateViNormalMode();

    bool handleKeypress( const QKeyEvent *e );

    bool commandEnterInsertMode();
    bool commandEnterInsertModeAppend();
    bool commandEnterInsertModeAppendEOL();
    bool commandEnterInsertModeBeforeFirstNonBlankInLine();
    bool commandEnterInsertModeLast();

    bool commandEnterVisualMode();
    bool commandEnterVisualLineMode();
    bool commandEnterVisualBlockMode();
    bool commandReselectVisual();
    bool commandToOtherEnd();

    bool commandEnterReplaceMode();

    bool commandDelete();
    bool commandDeleteToEOL();
    bool commandDeleteLine();

    bool commandMakeLowercase();
    bool commandMakeLowercaseLine();
    bool commandMakeUppercase();
    bool commandMakeUppercaseLine();
    bool commandChangeCase();

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

    bool commandPaste();
    bool commandPasteBefore();

    bool commandgPaste();
    bool commandgPasteBefore();

    bool commandDeleteChar();
    bool commandDeleteCharBackward();

    bool commandReplaceCharacter();

    bool commandSwitchToCmdLine();
    bool commandSearchBackward();
    bool commandSearchForward();
    bool commandUndo();
    bool commandRedo();

    bool commandSetMark();

    bool commandIndentLine();
    bool commandUnindentLine();
    bool commandIndentLines();
    bool commandUnindentLines();

    bool commandScrollPageDown();
    bool commandScrollPageUp();
    bool commandScrollHalfPageUp();
    bool commandScrollHalfPageDown();

    bool commandCentreViewOnCursor();

    bool commandAbort();

    bool commandPrintCharacterCode();

    bool commandRepeatLastChange();

    bool commandAlignLine();
    bool commandAlignLines();

    bool commandAddToNumber();
    bool commandSubtractFromNumber();

    bool commandPrependToBlock();
    bool commandAppendToBlock();

    bool commandGoToNextJump();
    bool commandGoToPrevJump();

    bool commandSwitchToLeftView();
    bool commandSwitchToUpView();
    bool commandSwitchToDownView();
    bool commandSwitchToRightView();
    bool commandSwitchToNextView();

    bool commandSplitHoriz();
    bool commandSplitVert();

    bool commandSwitchToNextTab();
    bool commandSwitchToPrevTab();

    bool commandFormatLine();
    bool commandFormatLines();

    bool commandCollapseToplevelNodes();
    bool commandCollapseLocal();
    bool commandExpandAll();
    bool commandExpandLocal();
    bool commandToggleRegionVisibility();

    // MOTIONS

    KateViRange motionLeft();
    KateViRange motionRight();
    KateViRange motionDown();
    KateViRange motionUp();

    KateViRange motionPageDown();
    KateViRange motionPageUp();

    KateViRange motionUpToFirstNonBlank();
    KateViRange motionDownToFirstNonBlank();

    KateViRange motionWordForward();
    KateViRange motionWordBackward();
    KateViRange motionWORDForward();
    KateViRange motionWORDBackward();

    KateViRange motionToEndOfWord();
    KateViRange motionToEndOfWORD();
    KateViRange motionToEndOfPrevWord();
    KateViRange motionToEndOfPrevWORD();

    KateViRange motionFindChar();
    KateViRange motionFindCharBackward();
    KateViRange motionToChar();
    KateViRange motionToCharBackward();
    KateViRange motionRepeatlastTF();
    KateViRange motionRepeatlastTFBackward();
    KateViRange motionFindNext();
    KateViRange motionFindPrev();


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

    KateViRange motionToNextOccurrence();
    KateViRange motionToPrevOccurrence();

    KateViRange motionToFirstLineOfWindow();
    KateViRange motionToMiddleLineOfWindow();
    KateViRange motionToLastLineOfWindow();

    KateViRange motionToNextVisualLine();
    KateViRange motionToPrevVisualLine();

    KateViRange motionToBeforeParagraph();
    KateViRange motionToAfterParagraph();

    // TEXT OBJECTS

    KateViRange textObjectAWord();
    KateViRange textObjectInnerWord();
    KateViRange textObjectAWORD();
    KateViRange textObjectInnerWORD();

    KateViRange textObjectAQuoteDouble();
    KateViRange textObjectInnerQuoteDouble();

    KateViRange textObjectAQuoteSingle();
    KateViRange textObjectInnerQuoteSingle();

    KateViRange textObjectABackQuote();
    KateViRange textObjectInnerBackQuote();

    KateViRange textObjectAParen();
    KateViRange textObjectInnerParen();

    KateViRange textObjectABracket();
    KateViRange textObjectInnerBracket();

    KateViRange textObjectACurlyBracket();
    KateViRange textObjectInnerCurlyBracket();

    KateViRange textObjectAInequalitySign();
    KateViRange textObjectInnerInequalitySign();

    KateViRange textObjectAComma();
    KateViRange textObjectInnerComma();

    void addCurrentPositionToJumpList();

    void addMapping( const QString &from, const QString &to );
    const QString getMapping( const QString &from ) const;
    const QStringList getMappings() const;
    virtual void reset();

    void setMappingTimeout(int timeoutMS);

    void beginMonitoringDocumentChanges();
  protected:
    void resetParser();
    void initializeCommands();
    QRegExp generateMatchingItemRegex();
    virtual void goToPos( const KateViRange &r );
    void executeCommand( const KateViCommand* cmd );
    OperationMode getOperationMode() const;
    // The 'current position' is the current cursor position for non-linewise pastes, and the current
    // line for linewise.
    enum PasteLocation { AtCurrentPosition, AfterCurrentPosition };
    bool paste(PasteLocation pasteLocation, bool isgPaste);
    Cursor cursorPosAtEndOfPaste(const Cursor& pasteLocation, const QString& pastedText);

    void joinLines(unsigned int from, unsigned int to) const;
    void reformatLines(unsigned int from, unsigned int to) const;

    KateViRange textObjectComma(bool inner);
    void shrinkRangeAroundCursor(KateViRange& toShrink, const KateViRange& rangeToShrinkTo);

    QString m_keys;
    unsigned int m_countTemp;
    bool m_findWaitingForChar;

    QVector<KateViCommand *> m_commands;
    QVector<KateViMotion *> m_motions;
    QVector<int> m_matchingCommands;
    QVector<int> m_matchingMotions;
    QStack<int> m_awaitingMotionOrTextObject;

    int m_motionOperatorIndex;

    QString m_lastTFcommand; // holds the last t/T/f/F command so that it can be repeated with ;/,
    bool m_isRepeatedTFcommand;

    bool m_linewiseCommand;
    bool m_commandWithMotion;

    bool m_commandShouldKeepSelection;

    bool m_deleteCommand;

    uint m_scroll_count_limit;

    // registers
    QChar m_defaultRegister;
    QString m_registerTemp;

    // item matching ('%' motion)
    QHash<QString, QString> m_matchingItems;
    QRegExp m_matchItemRegex;

    // mappings
    bool m_mappingKeyPress;
    // Will be the mapping used if we decide that no extra mapping characters will be
    // typed, either because we have a mapping that cannot be extended to another
    // mapping by adding additional characters, or we have a mapping and timed out waiting
    // for it to be extended to a longer mapping.
    // (Essentially, this allows us to have mappings that extend each other e.g. "'12" and
    // "'123", and to choose between them.)
    QString m_fullMappingMatch;
    // set after f/F/t/T/r so the following character isn't translated
    bool m_ignoreMapping;
    QString m_mappingKeys;
    void executeMapping();

    KateViKeyParser *m_keyParser;

    // Ctrl-c or ESC have been pressed, leading to a call to reset().
    bool m_pendingResetIsDueToExit;

    Cursor m_currentChangeEndMarker;
private slots:
    void textInserted(KTextEditor::Document* document, KTextEditor::Range range);
    void textRemoved(KTextEditor::Document*,KTextEditor::Range);
};

#endif
