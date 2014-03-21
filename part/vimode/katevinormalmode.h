/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 - 2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2009 Paul Gideon Dann <pdgiddie@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
 *  Copyright (C) 2012 - 2013 Simon St James <kdedevel@etotheipiplusone.com>
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
    bool commandChangeCaseRange();
    bool commandChangeCaseLine();

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

    bool commandIndentedPaste();
    bool commandIndentedPasteBefore();

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

    bool commandStartRecordingMacro();
    bool commandReplayMacro();

    bool commandCloseWrite();
    bool commandCloseNocheck();

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

    KateViRange motionToPreviousSentence();
    KateViRange motionToNextSentence();

    KateViRange motionToBeforeParagraph();
    KateViRange motionToAfterParagraph();

    KateViRange motionToIncrementalSearchMatch();

    // TEXT OBJECTS

    KateViRange textObjectAWord();
    KateViRange textObjectInnerWord();
    KateViRange textObjectAWORD();
    KateViRange textObjectInnerWORD();

    KateViRange textObjectInnerSentence();
    KateViRange textObjectASentence();

    KateViRange textObjectInnerParagraph();
    KateViRange textObjectAParagraph();

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

    virtual void reset();

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
    bool paste(KateViNormalMode::PasteLocation pasteLocation, bool isgPaste, bool isIndentedPaste);
    Cursor cursorPosAtEndOfPaste(const Cursor& pasteLocation, const QString& pastedText);

    void joinLines(unsigned int from, unsigned int to) const;
    void reformatLines(unsigned int from, unsigned int to) const;

    /**
     * Get the index of the first non-blank character from the given line.
     *
     * @param line The line to be picked. The current line will picked instead
     * if this parameter is set to a negative value.
     * @returns the index of the first non-blank character from the given line.
     * If a non-space character cannot be found, the 0 is returned.
     */
    int getFirstNonBlank(int line = -1) const;

    KateViRange textObjectComma(bool inner);
    void shrinkRangeAroundCursor(KateViRange& toShrink, const KateViRange& rangeToShrinkTo);
    Cursor findSentenceStart();
    Cursor findSentenceEnd();
    Cursor findParagraphStart();
    Cursor findParagraphEnd();

    QString m_keys;
    unsigned int m_countTemp;
    bool m_findWaitingForChar;

    QVector<KateViCommand *> m_commands;
    QVector<KateViMotion *> m_motions;
    QVector<int> m_matchingCommands;
    QVector<int> m_matchingMotions;
    QStack<int> m_awaitingMotionOrTextObject;
    bool motionWillBeUsedWithCommand() { return !m_awaitingMotionOrTextObject.isEmpty(); };

    int m_motionOperatorIndex;

    QString m_lastTFcommand; // holds the last t/T/f/F command so that it can be repeated with ;/,
    bool m_isRepeatedTFcommand;

    bool m_linewiseCommand;
    bool m_commandWithMotion;
    bool m_lastMotionWasLinewiseInnerBlock;
    bool m_motionCanChangeWholeVisualModeSelection;

    bool m_commandShouldKeepSelection;

    bool m_deleteCommand;

    uint m_scroll_count_limit;

    // registers
    QChar m_defaultRegister;
    QString m_registerTemp;

    // item matching ('%' motion)
    QHash<QString, QString> m_matchingItems;
    QRegExp m_matchItemRegex;

    KateViKeyParser *m_keyParser;

    // Ctrl-c or ESC have been pressed, leading to a call to reset().
    bool m_pendingResetIsDueToExit;

    KTextEditor::Attribute::Ptr m_highlightYankAttribute;
    QSet<KTextEditor::MovingRange *> m_highlightedYanks;

    void highlightYank(const KateViRange& range, const OperationMode mode = CharWise);
    void addHighlightYank(const Range& range);
    QSet<KTextEditor::MovingRange *> &highlightedYankForDocument();

    Cursor m_currentChangeEndMarker;

    bool m_isUndo;

    Cursor m_positionWhenIncrementalSearchBegan;

    bool waitingForRegisterOrCharToSearch();
private slots:
    void textInserted(KTextEditor::Document* document, KTextEditor::Range range);
    void textRemoved(KTextEditor::Document*,KTextEditor::Range);
    void undoBeginning();
    void undoEnded();

    void updateYankHighlightAttrib();
    void clearYankHighlight();
    void aboutToDeleteMovingInterfaceContent();
};

#endif
