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
  explicit KateViEmulatedCommandBar(KateView *view, QWidget* parent = 0);
  virtual ~KateViEmulatedCommandBar();
  void init(bool backwards);
  bool isActive();
  virtual void closed();
  bool handleKeyPress(const QKeyEvent* keyEvent);
private:
  bool m_isActive;
  KateView *m_view;
  QLineEdit *m_edit;
  QLabel *m_barTypeIndicator;
  bool m_searchBackwards;
  KTextEditor::Cursor m_startingCursorPos;
  bool m_doNotResetCursorOnClose;
  bool m_suspendEditEventFiltering;
  bool m_waitingForRegister;

  QCompleter *m_completer;
  QStringListModel *m_searchHistoryModel;
  enum CompletionType { None, SearchHistory, WordFromDocument };
  CompletionType m_currentCompletionType;
  void updateCompletionPrefix();
  void completionChosen();
  bool m_completionActive;

  KTextEditor::Attribute::Ptr m_highlightMatchAttribute;
  KTextEditor::MovingRange* m_highlightedMatch;
  void updateMatchHighlight(const KTextEditor::Range& matchRange);

  virtual bool eventFilter(QObject* object, QEvent* event);
  void deleteSpacesToLeftOfCursor();
  void deleteWordCharsToLeftOfCursor();
  bool deleteNonWordCharsToLeftOfCursor();
  QString wordBeforeCursor();
  void replaceWordBeforeCursorWith(const QString& newWord);
  QString vimRegexToQtRegexPattern(const QString& vimRegexPattern);
  QString toggledEscaped(const QString& string, QChar escapeChar);
  QString ensuredCharEscaped(const QString& string, QChar charToEscape);

  void populateAndShowSearchHistoryCompletion();
  void populateAndShowWordFromDocumentCompletion();
  void setCompletionIndex(int index);
private slots:
  void editTextChanged(const QString& newText);
  void updateMatchHighlightAttrib();
};

#endif