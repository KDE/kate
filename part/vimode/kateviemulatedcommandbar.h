/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2013 Simon St James <kdedevel@etotheipiplusone.com>
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
#ifndef KATEVIEMULATEDCOMMANDBAR_H
#define KATEVIEMULATEDCOMMANDBAR_H

#include "kateviewhelpers.h"
#include "katecmds.h"
#include <ktexteditor/cursor.h>
#include <ktexteditor/attribute.h>
#include <ktexteditor/movingrange.h>

class KateView;
class QLabel;
class QCompleter;
class QStringListModel;

/**
 * A KateViewBarWidget that attempts to emulate some of the features of Vim's own command bar,
 * including insertion of register contents via ctr-r<registername>; dismissal via
 * ctrl-c and ctrl-[; bi-directional incremental searching, with SmartCase; interactive sed-replace;
 * plus a few extensions such as completion from document and navigable sed search and sed replace history.
 */
class KATEPART_TESTS_EXPORT KateViEmulatedCommandBar : public KateViewBarWidget
{
  Q_OBJECT
public:
  enum Mode { NoMode, SearchForward, SearchBackward, Command };
  explicit KateViEmulatedCommandBar(KateView *view, QWidget* parent = 0);
  virtual ~KateViEmulatedCommandBar();
  void init(Mode mode, const QString& initialText = QString());
  bool isActive();
  void setCommandResponseMessageTimeout(long commandResponseMessageTimeOutMS);
  virtual void closed();
  bool handleKeyPress(const QKeyEvent* keyEvent);
  bool isSendingSyntheticSearchCompletedKeypress();

  void startInteractiveSearchAndReplace(QSharedPointer<KateCommands::SedReplace::InteractiveSedReplacer> interactiveSedReplace);

private:
  bool m_isActive;
  Mode m_mode;
  KateView *m_view;
  QLineEdit *m_edit;
  QLabel *m_barTypeIndicator;
  void showBarTypeIndicator(Mode mode);
  KTextEditor::Cursor m_startingCursorPos;
  bool m_wasAborted;
  bool m_suspendEditEventFiltering;
  bool m_waitingForRegister;
  QLabel *m_waitingForRegisterIndicator;
  bool m_insertedTextShouldBeEscapedForSearchingAsLiteral;

  QTimer *m_commandResponseMessageDisplayHide;
  QLabel* m_commandResponseMessageDisplay;
  long m_commandResponseMessageTimeOutMS;
  QString executeCommand(const QString& commandToExecute);
  void switchToCommandResponseDisplay(const QString& commandResponseMessage);

  QLabel *m_interactiveSedReplaceLabel;
  bool m_interactiveSedReplaceActive;
  void updateInteractiveSedReplaceLabelText();
  QSharedPointer<KateCommands::SedReplace::InteractiveSedReplacer> m_interactiveSedReplacer;
  void finishInteractiveSedReplace();

  void moveCursorTo(const KTextEditor::Cursor& cursorPos);

  QCompleter *m_completer;
  QStringListModel *m_completionModel;
  bool m_isNextTextChangeDueToCompletionChange;
  enum CompletionType { None, SearchHistory, WordFromDocument, Commands, CommandHistory, SedFindHistory, SedReplaceHistory };
  CompletionType m_currentCompletionType;
  void updateCompletionPrefix();
  void currentCompletionChanged();
  bool m_completionActive;
  QString m_textToRevertToIfCompletionAborted;
  int m_cursorPosToRevertToIfCompletionAborted;

  KTextEditor::Attribute::Ptr m_highlightMatchAttribute;
  KTextEditor::MovingRange* m_highlightedMatch;
  void updateMatchHighlight(const KTextEditor::Range& matchRange);
  enum BarBackgroundStatus { Normal, MatchFound, NoMatchFound };
  void setBarBackground(BarBackgroundStatus status);

  QString m_currentSearchPattern;
  bool m_currentSearchIsCaseSensitive;
  bool m_currentSearchIsBackwards;
  bool m_currentSearchPlacesCursorAtEndOfMatch;

  bool m_isSendingSyntheticSearchCompletedKeypress;

  virtual bool eventFilter(QObject* object, QEvent* event);
  void deleteSpacesToLeftOfCursor();
  void deleteWordCharsToLeftOfCursor();
  bool deleteNonWordCharsToLeftOfCursor();
  QString wordBeforeCursor();
  QString commandBeforeCursor();
  void replaceWordBeforeCursorWith(const QString& newWord);
  void replaceCommandBeforeCursorWith(const QString& newCommand);

  void activateSearchHistoryCompletion();
  void activateWordFromDocumentCompletion();
  void activateCommandCompletion();
  void activateCommandHistoryCompletion();
  void activateSedFindHistoryCompletion();
  void activateSedReplaceHistoryCompletion();
  void deactivateCompletion();
  void abortCompletionAndResetToPreCompletion();
  void setCompletionIndex(int index);

  /**
   * Stuff to do with expressions of the form:
   *
   *   s/find/replace/<sedflags>
   */
  struct ParsedSedExpression
  {
    bool parsedSuccessfully;
    int findBeginPos;
    int findEndPos;
    int replaceBeginPos;
    int replaceEndPos;
    QChar delimiter;
  };
  ParsedSedExpression parseAsSedExpression();
  QString withSedFindTermReplacedWith(const QString& newFindTerm);
  QString withSedReplaceTermReplacedWith(const QString& newReplaceTerm);
  QString sedFindTerm();
  QString sedReplaceTerm();
  QString withSedDelimiterEscaped(const QString& text);

  bool isCursorInFindTermOfSed();
  bool isCursorInReplaceTermOfSed();

  /**
   * The "range expression" is the (optional) expression before the command that describes
   * the range over which the command should be run e.g. '<,'>.  @see CommandRangeExpressionParser
   */
  QString withoutRangeExpression();
  QString rangeExpression();
private slots:
  void editTextChanged(const QString& newText);
  void updateMatchHighlightAttrib();
  void startHideCommandResponseTimer();
};

#endif

