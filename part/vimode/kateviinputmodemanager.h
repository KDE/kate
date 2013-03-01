/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
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
class KateViNormalMode;
class KateViInsertMode;
class KateViVisualMode;
class KateViReplaceMode;
class KateViKeyParser;
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
   * @return The current vi mode
   */
  ViMode getCurrentViMode() const;

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
  bool isReplayingLastChange() const { return m_replayingLastChange; }

  /**
   * set whether insert mode should be repeated as text instead of
   * keystrokes. the default is keystrokes, which is reset between
   * changes. for example, when invoking a completion command in
   * insert mode, textual repeat should be enabled since repeating the
   * completion may yield different results elsewhere in the document.
   * therefore, it is better to repeat the text itself. however, most
   * insertion commands do not need textual repeat; it should only be
   * used when the repeating of keystrokes leads to unpredictable
   * results. indeed, in most cases, repeating the keystrokes is the
   * most predictable approach.
   * @see isTextualRepeat()
   * @return true if insert mode should be repeated as text
   */
  void setTextualRepeat( bool b = true ) { m_textualRepeat = b; }

  /**
   * whether insert mode should be repeated as text instead of keystrokes.
   * @see setTextualRepeat()
   * @return true if insert mode should be repeated as text
   */
  bool isTextualRepeat() const { return m_textualRepeat; }

  /**
   * append a QKeyEvent to the key event log
   */
  void appendKeyEventToLog(const QKeyEvent &e);

  /**
   * clear the key event log
   */
  void clearLog() { m_keyEventsLog.clear(); m_keyEventsBeforeInsert.clear(); }

  /**
   * copy the contents of the key events log to m_lastChange so that it can be repeated
   */
  void storeChangeCommand();

  /**
   * repeat last change by feeding the contents of m_lastChange to feedKeys()
   */
  void repeatLastChange(int count);

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

  KateView *m_view;
  KateViewInternal *m_viewInternal;
  KateViKeyParser *m_keyParser;

  /**
   * set to true when replaying the last change (due to e.g. pressing ".")
   */
  bool m_replayingLastChange;

  /**
   * set to true when the insertion should be repeated as text
   */
  bool m_textualRepeat;

  /**
   * a continually updated list of the key events that was part of the last change.
   * updated until copied to m_lastChange when the change is completed.
   */
  QList<QKeyEvent> m_keyEventsLog;

  /**
   * like m_keyEventsLog, but excludes insert mode: that is, only the key events
   * leading up to insert mode (like "i", "cw", and "o") are logged.
   */
  QList<QKeyEvent> m_keyEventsBeforeInsert;

  /**
   * a list of the key events that was part of the last change.
   */
  QString m_lastChange;

  /**
   * keeps track of whether the last search was done backwards or not.
   */
  bool m_lastSearchBackwards;

  /**
   * true when normal mode was started by Ctrl-O command in insert mode.
   */
  bool m_temporaryNormalMode;

  /**
   * true when mark set inside viinputmodemanager to do not serve it as bookmark set;
   */
  bool m_mark_set_inside_viinputmodemanager;

  // jump list
  QList<KateViJump> *jump_list;
  QList<KateViJump>::iterator current_jump;

  // marks
  QMap<QChar, KTextEditor::MovingCursor*> m_marks;

};

#endif
