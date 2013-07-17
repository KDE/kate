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
#include <ktexteditor/cursor.h>
#include <ktexteditor/attribute.h>
#include <ktexteditor/movingrange.h>

class KateView;
class QLabel;
class QCompleter;
class QStringListModel;

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
private:
  bool m_isActive;
  Mode m_mode;
  KateView *m_view;
  QLineEdit *m_edit;
  QLabel *m_barTypeIndicator;
  KTextEditor::Cursor m_startingCursorPos;
  bool m_wasAborted;
  bool m_suspendEditEventFiltering;
  bool m_waitingForRegister;
  QLabel* m_commandResponseMessageDisplay;
  long m_commandResponseMessageTimeOutMS;
  QString m_commandResponseMessage;

  QCompleter *m_completer;
  QStringListModel *m_completionModel;
  bool m_nextTextChangeDueToCompletionChange;
  enum CompletionType { None, SearchHistory, WordFromDocument, Commands, CommandHistory };
  CompletionType m_currentCompletionType;
  void updateCompletionPrefix();
  void currentCompletionChanged();
  bool m_completionActive;
  QString m_revertToIfCompletionAborted;

  KTextEditor::Attribute::Ptr m_highlightMatchAttribute;
  KTextEditor::MovingRange* m_highlightedMatch;
  void updateMatchHighlight(const KTextEditor::Range& matchRange);

  QString m_currentSearchPattern;
  bool m_currentSearchIsCaseSensitive;
  bool m_currentSearchIsBackwards;

  virtual bool eventFilter(QObject* object, QEvent* event);
  void deleteSpacesToLeftOfCursor();
  void deleteWordCharsToLeftOfCursor();
  bool deleteNonWordCharsToLeftOfCursor();
  QString wordBeforeCursor();
  QString commandBeforeCursor();
  void replaceWordBeforeCursorWith(const QString& newWord);

  void activateSearchHistoryCompletion();
  void activateWordFromDocumentCompletion();
  void activateCommandCompletion();
  void activateCommandHistoryCompletion();
  void deactivateCompletion();
  void setCompletionIndex(int index);
private slots:
  void editTextChanged(const QString& newText);
  void updateMatchHighlightAttrib();
};

#endif