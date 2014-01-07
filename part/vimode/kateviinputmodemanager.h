/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
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

#ifndef KATE_VI_INPUT_MODE_MANAGER_INCLUDED
#define KATE_VI_INPUT_MODE_MANAGER_INCLUDED

#include <QKeyEvent>
#include <QList>
#include "katepartprivate_export.h"
#include <ktexteditor/cursor.h>
#include "katedocument.h"

class KConfigGroup;
class KateView;
class KateDocument;
class KateViewInternal;
class KateViModeBase;
class KateViNormalMode;
class KateViInsertMode;
class KateViVisualMode;
class KateViReplaceMode;
class KateViKeyParser;
class KateViKeyMapper;
class QString;

/**
 * The four vi modes supported by Kate's vi input mode
 */
enum ViMode {
  NormalMode,
  InsertMode,
  VisualMode,
  VisualLineMode,
  VisualBlockMode,
  ReplaceMode
};

struct KateViJump {
    int line;
    int column;
};

namespace KTextEditor {
  class MovingCursor;
  class Mark;
  class MarkInterface;
}

class KATEPART_TESTS_EXPORT KateViInputModeManager : public QObject
{
    Q_OBJECT

public:
  KateViInputModeManager(KateView* view, KateViewInternal* viewInternal);
  ~KateViInputModeManager();

  /**
   * feed key the given key press to the command parser
   * @return true if keypress was is [part of a] command, false otherwise
   */
  bool handleKeypress(const QKeyEvent *e);

  /**
   * feed key the given list of key presses to the key handling code, one by one
   */
  void feedKeyPresses(const QString &keyPresses) const;

  /**
   * Determines whether we are currently processing a Vi keypress
   * @return true if we are still in a call to handleKeypress, false otherwise
   */
  bool isHandlingKeypress() const;

  /**
   * @return The current vi mode
   */
  ViMode getCurrentViMode() const;

  /**
   * @return the previous vi mode
   */
  ViMode getPreviousViMode() const;

  /**
   * @return true if and only if the current mode is one of VisualMode, VisualBlockMode or VisualLineMode.
   */
  bool isAnyVisualMode() const;

  /**
   * @return one of getViNormalMode(), getViVisualMode(), etc, depending on getCurrentViMode().
   */
  KateViModeBase* getCurrentViModeHandler() const;

  const QString getVerbatimKeys() const;

  /**
   * changes the current vi mode to the given mode
   */
  void changeViMode(ViMode newMode);

  /**
   * set normal mode to be the active vi mode and perform the needed setup work
   */
  void viEnterNormalMode();

  /**
   * set insert mode to be the active vi mode and perform the needed setup work
   */
  void viEnterInsertMode();

  /**
   * set visual mode to be the active vi mode and make the needed setup work
   */
  void viEnterVisualMode( ViMode visualMode = VisualMode );

  /**
   * set replace mode to be the active vi mode and make the needed setup work
   */
  void viEnterReplaceMode();

  /**
   * @return the KateViNormalMode instance
   */
  KateViNormalMode* getViNormalMode();

  /**
   * @return the KateViInsertMode instance
   */
  KateViInsertMode* getViInsertMode();

  /**
   * @return the KateViVisualMode instance
   */
  KateViVisualMode* getViVisualMode();

  /**
   * @return the KateViReplaceMode instance
   */
  KateViReplaceMode* getViReplaceMode();

  /**
   * @return true if running replaying the last change due to pressing "."
   */
  bool isReplayingLastChange() const { return m_isReplayingLastChange; }

  /**
   * append a QKeyEvent to the key event log
   */
  void appendKeyEventToLog(const QKeyEvent &e);

  /**
   * clear the key event log
   */
  void clearCurrentChangeLog() { m_currentChangeKeyEventsLog.clear(); m_currentChangeCompletionsLog.clear(); }

  /**
   * copy the contents of the key events log to m_lastChange so that it can be repeated
   */
  void storeLastChangeCommand();

  /**
   * repeat last change by feeding the contents of m_lastChange to feedKeys()
   */
  void repeatLastChange();

  class Completion
  {
  public:
    enum CompletionType { PlainText, FunctionWithoutArgs, FunctionWithArgs };
    Completion(const QString& completedText, bool removeTail, CompletionType completionType);
    QString completedText() const;
    bool removeTail() const;
    CompletionType completionType() const;
  private:
    QString m_completedText;
    bool m_removeTail;
    CompletionType m_completionType;
  };
  void startRecordingMacro(QChar macroRegister);
  void finishRecordingMacro();
  bool isRecordingMacro();
  void replayMacro(QChar macroRegister);
  bool isReplayingMacro();
  void logCompletionEvent(const Completion& completion);
  Completion nextLoggedCompletion();
  void doNotLogCurrentKeypress();

  /**
   * The current search pattern.
   * This is set by the last search.
   * @return the search pattern or the empty string if not set
   */
  const QString getLastSearchPattern() const;

  /**
   * Set the current search pattern.
   * This is used by the "n" and "N" motions.
   * @param p the search pattern
   */
  void setLastSearchPattern( const QString &p );

  /**
   * get search direction of last search. (true if backwards, false if forwards)
   */
  bool lastSearchBackwards() const { return m_lastSearchBackwards; }

  /**
   * set search direction of last search. (true if backwards, false if forwards)
   */
  void setLastSearchBackwards( bool b ) { m_lastSearchBackwards = b; }

  void setLastSearchCaseSensitive(bool caseSensitive) { m_lastSearchCaseSensitive = caseSensitive; };

  void setLastSearchPlacesCursorAtEndOfMatch(bool b) { m_lastSearchPlacedCursorAtEndOfMatch = b; };

  bool lastSearchCaseSensitive() { return m_lastSearchCaseSensitive; };

  bool lastSearchPlacesCursorAtEndOfMatch() { return m_lastSearchPlacedCursorAtEndOfMatch; };

  bool getTemporaryNormalMode() { return m_temporaryNormalMode; }

  void setTemporaryNormalMode(bool b) {  m_temporaryNormalMode = b; }

  void reset();

  // Jump Lists
  void addJump(KTextEditor::Cursor cursor);
  KTextEditor::Cursor getNextJump(KTextEditor::Cursor cursor);
  KTextEditor::Cursor getPrevJump(KTextEditor::Cursor cursor);
  void PrintJumpList();

  // session stuff
  void readSessionConfig( const KConfigGroup& config );
  void writeSessionConfig( KConfigGroup& config );

  // marks
  /**
   * Add a mark to the document.
   * By default, the mark is made visible in the document
   * by highlighting its line, and it moves when inserting
   * text at it.
   * @param doc the document to insert the mark into
   * @param mark the mark's name
   * @param pos the mark's position
   * @param moveoninsert whether the mark should move or stay behind
   *        when inserting text at it
   * @param showmark whether to highlight the mark's line
   */
  void addMark( KateDocument* doc, const QChar& mark, const KTextEditor::Cursor& pos,
                const bool moveoninsert = true, const bool showmark = true );
  KTextEditor::Cursor getMarkPosition( const QChar& mark ) const;
  void syncViMarksAndBookmarks();
  QString getMarksOnTheLine(int line);

  /**
   * convert mode to string representation for user
   * @param mode mode enum value
   * @return user visible string
   */
  static QString modeToString(ViMode mode);

  KateViKeyMapper* keyMapper();

private Q_SLOTS:
  void markChanged (KTextEditor::Document* doc,
                    KTextEditor::Mark mark,
                    KTextEditor::MarkInterface::MarkChangeAction action);

private:
  KateViNormalMode* m_viNormalMode;
  KateViInsertMode* m_viInsertMode;
  KateViVisualMode* m_viVisualMode;
  KateViReplaceMode* m_viReplaceMode;

  ViMode m_currentViMode;
  ViMode m_previousViMode;

  KateView *m_view;
  KateViewInternal *m_viewInternal;
  KateViKeyParser *m_keyParser;

  // Create a new keymapper for each macro event, to simplify expansion of mappings in macros
  // where the macro itself was triggered by expanding a mapping!
  QStack<QSharedPointer<KateViKeyMapper> > m_keyMapperStack;

  int m_insideHandlingKeyPressCount;

  /**
   * set to true when replaying the last change (due to e.g. pressing ".")
   */
  bool m_isReplayingLastChange;

  bool m_isRecordingMacro;

  QChar m_recordingMacroRegister;
  QList<QKeyEvent> m_currentMacroKeyEventsLog;

  int m_macrosBeingReplayedCount;
  QChar m_lastPlayedMacroRegister;

  QList<Completion> m_currentMacroCompletionsLog;

  /**
   * Stuff for retrieving the next completion for the macro.
   * Needs to be on a stack for if macros call other macros
   * which have their own stored completions.
   */
  QStack<QList<Completion> > m_macroCompletionsToReplay;
  QStack< int > m_nextLoggedMacroCompletionIndex;

  /**
   * a continually updated list of the key events that was part of the last change.
   * updated until copied to m_lastChange when the change is completed.
   */
  QList<QKeyEvent> m_currentChangeKeyEventsLog;
  QList<Completion> m_currentChangeCompletionsLog;
  QList<Completion> m_lastChangeCompletionsLog;
  int m_nextLoggedLastChangeComplexIndex;

  /**
   * a list of the (encoded) key events that was part of the last change.
   */
  QString m_lastChange;

  QString m_lastSearchPattern;
  /**
   * keeps track of whether the last search was done backwards or not.
   */
  bool m_lastSearchBackwards;
  /**
   * keeps track of whether the last search was case-sensitive or not.
   */
  bool m_lastSearchCaseSensitive;

  bool m_lastSearchPlacedCursorAtEndOfMatch;

  /**
   * true when normal mode was started by Ctrl-O command in insert mode.
   */
  bool m_temporaryNormalMode;

  /**
   * true when mark set inside viinputmodemanager to do not serve it as bookmark set;
   */
  bool m_markSetInsideViInputModeManager;

  // jump list
  QList<KateViJump> *jump_list;
  QList<KateViJump>::iterator current_jump;

  // marks
  QMap<QChar, KTextEditor::MovingCursor*> m_marks;

};

#endif
