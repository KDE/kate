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
#include "kateviemulatedcommandbar.h"
#include "katevikeyparser.h"
#include "kateview.h"
#include "kateviglobal.h"
#include "katevinormalmode.h"
#include "katevivisualmode.h"
#include "kateglobal.h"
#include "kateconfig.h"
#include "katecmd.h"
#include <katecmds.h>

#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QCompleter>
#include <QApplication>
#include <KDE/KColorScheme>
#include <algorithm>

namespace
{
  bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
  {
    return s1.toLower() < s2.toLower();
  }

  bool isCharEscaped(const QString& string, int charPos)
  {
    if (charPos == 0)
    {
      return false;
    }
    int numContiguousBackslashesToLeft = 0;
    charPos--;
    while (charPos >= 0 && string[charPos] == '\\')
    {
      numContiguousBackslashesToLeft++;
      charPos--;
    }
    return ((numContiguousBackslashesToLeft % 2) == 1);
  }

  QString toggledEscaped(const QString& originalString, QChar escapeChar)
  {
    int searchFrom = 0;
    QString toggledEscapedString = originalString;
    do
    {
      int indexOfEscapeChar = toggledEscapedString.indexOf(escapeChar , searchFrom);
      if (indexOfEscapeChar == -1)
      {
        break;
      }
      if (!isCharEscaped(toggledEscapedString, indexOfEscapeChar))
      {
        // Escape.
        toggledEscapedString.replace(indexOfEscapeChar, 1, QString("\\") + escapeChar);
        searchFrom = indexOfEscapeChar + 2;
      }
      else
      {
        // Unescape.
        toggledEscapedString.remove(indexOfEscapeChar - 1, 1);
        searchFrom = indexOfEscapeChar;
      }
    } while (true);

    return toggledEscapedString;
  }

  QString ensuredCharEscaped(const QString& originalString, QChar charToEscape)
  {
      QString escapedString = originalString;
      for (int i = 0; i < escapedString.length(); i++)
      {
        if (escapedString[i] == charToEscape && !isCharEscaped(escapedString, i))
        {
          escapedString.replace(i, 1, QString("\\") + charToEscape);
        }
      }
      return escapedString;
  }

  QString vimRegexToQtRegexPattern(const QString& vimRegexPattern)
  {
    QString qtRegexPattern = vimRegexPattern;
    qtRegexPattern = toggledEscaped(qtRegexPattern, '(');
    qtRegexPattern = toggledEscaped(qtRegexPattern, ')');
    qtRegexPattern = toggledEscaped(qtRegexPattern, '+');
    qtRegexPattern = toggledEscaped(qtRegexPattern, '|');
    {
      // All curly brackets, except the closing curly bracket of a matching pair where the opening bracket is escaped,
      // must have their escaping toggled.
      bool lookingForMatchingCloseBracket = false;
      QList<int> matchingClosedCurlyBracketPositions;
      for (int i = 0; i < qtRegexPattern.length(); i++)
      {
        if (qtRegexPattern[i] == '{' && i > 0 && qtRegexPattern[i - 1] == '\\')
        {
          lookingForMatchingCloseBracket = true;
        }
        if (qtRegexPattern[i] == '}' && lookingForMatchingCloseBracket && qtRegexPattern[i - 1] != '\\')
        {
          matchingClosedCurlyBracketPositions.append(i);
        }
      }
      if (matchingClosedCurlyBracketPositions.isEmpty())
      {
        // Escape all {'s and }'s - there are no matching pairs.
        qtRegexPattern = toggledEscaped(qtRegexPattern, '{');
        qtRegexPattern = toggledEscaped(qtRegexPattern, '}');
      }
      else
      {
        // Ensure that every chunk of qtRegexPattern that does *not* contain a curly closing bracket
        // that is matched have their { and } escaping toggled.
        QString qtRegexPatternNonMatchingCurliesToggled;
        int previousNonMatchingClosedCurlyPos = 0; // i.e. the position of the last character which is either
                                                   // not a curly closing bracket, or is a curly closing bracket
                                                   // that is not matched.
        foreach (int matchingClosedCurlyPos, matchingClosedCurlyBracketPositions)
        {
          QString chunkExcludingMatchingCurlyClosed = qtRegexPattern.mid(previousNonMatchingClosedCurlyPos, matchingClosedCurlyPos - previousNonMatchingClosedCurlyPos);
          chunkExcludingMatchingCurlyClosed = toggledEscaped(chunkExcludingMatchingCurlyClosed, '{');
          chunkExcludingMatchingCurlyClosed = toggledEscaped(chunkExcludingMatchingCurlyClosed, '}');
          qtRegexPatternNonMatchingCurliesToggled += chunkExcludingMatchingCurlyClosed +
                                                     qtRegexPattern[matchingClosedCurlyPos];
          previousNonMatchingClosedCurlyPos = matchingClosedCurlyPos + 1;
        }
        QString chunkAfterLastMatchingClosedCurly = qtRegexPattern.mid(matchingClosedCurlyBracketPositions.last() + 1);
        chunkAfterLastMatchingClosedCurly = toggledEscaped(chunkAfterLastMatchingClosedCurly, '{');
        chunkAfterLastMatchingClosedCurly = toggledEscaped(chunkAfterLastMatchingClosedCurly, '}');
        qtRegexPatternNonMatchingCurliesToggled += chunkAfterLastMatchingClosedCurly;

        qtRegexPattern = qtRegexPatternNonMatchingCurliesToggled;
      }

    }

    // All square brackets, *except* for those that are a) unescaped; and b) form a matching pair, must be
    // escaped.
    bool lookingForMatchingCloseBracket = false;
    int openingBracketPos = -1;
    QList<int> matchingSquareBracketPositions;
    for (int i = 0; i < qtRegexPattern.length(); i++)
    {
      if (qtRegexPattern[i] == '[' && !isCharEscaped(qtRegexPattern, i) && !lookingForMatchingCloseBracket)
      {
        lookingForMatchingCloseBracket = true;
        openingBracketPos = i;
      }
      if (qtRegexPattern[i] == ']' && lookingForMatchingCloseBracket && !isCharEscaped(qtRegexPattern, i))
      {
        lookingForMatchingCloseBracket = false;
        matchingSquareBracketPositions.append(openingBracketPos);
        matchingSquareBracketPositions.append(i);
      }
    }

    if (matchingSquareBracketPositions.isEmpty())
    {
      // Escape all ['s and ]'s - there are no matching pairs.
      qtRegexPattern = ensuredCharEscaped(qtRegexPattern, '[');
      qtRegexPattern = ensuredCharEscaped(qtRegexPattern, ']');
    }
    else
    {
      // Ensure that every chunk of qtRegexPattern that does *not* contain one of the matching pairs of
      // square brackets have their square brackets escaped.
      QString qtRegexPatternNonMatchingSquaresMadeLiteral;
      int previousNonMatchingSquareBracketPos = 0; // i.e. the position of the last character which is
                                                   // either not a square bracket, or is a square bracket but
                                                   // which is not matched.
      foreach (int matchingSquareBracketPos, matchingSquareBracketPositions)
      {
        QString chunkExcludingMatchingSquareBrackets = qtRegexPattern.mid(previousNonMatchingSquareBracketPos, matchingSquareBracketPos - previousNonMatchingSquareBracketPos);
        chunkExcludingMatchingSquareBrackets = ensuredCharEscaped(chunkExcludingMatchingSquareBrackets, '[');
        chunkExcludingMatchingSquareBrackets = ensuredCharEscaped(chunkExcludingMatchingSquareBrackets, ']');
        qtRegexPatternNonMatchingSquaresMadeLiteral += chunkExcludingMatchingSquareBrackets + qtRegexPattern[matchingSquareBracketPos];
        previousNonMatchingSquareBracketPos = matchingSquareBracketPos + 1;
      }
      QString chunkAfterLastMatchingSquareBracket = qtRegexPattern.mid(matchingSquareBracketPositions.last() + 1);
      chunkAfterLastMatchingSquareBracket = ensuredCharEscaped(chunkAfterLastMatchingSquareBracket, '[');
      chunkAfterLastMatchingSquareBracket = ensuredCharEscaped(chunkAfterLastMatchingSquareBracket, ']');
      qtRegexPatternNonMatchingSquaresMadeLiteral += chunkAfterLastMatchingSquareBracket;

      qtRegexPattern = qtRegexPatternNonMatchingSquaresMadeLiteral;
    }


    qtRegexPattern = qtRegexPattern.replace("\\>", "\\b");
    qtRegexPattern = qtRegexPattern.replace("\\<", "\\b");

    return qtRegexPattern;
  }

  QStringList reversed(const QStringList& originalList)
  {
    QStringList reversedList = originalList;
    std::reverse(reversedList.begin(), reversedList.end());
    return reversedList;
  }

  class RangeExpressionParser
  {
  public:
      RangeExpressionParser();
      Range parseRangeExpression(const QString& command, QString& destRangeExpression, QString& destTransformedCommand, KateView* view);
  private:
    int calculatePosition( const QString& string, KateView *view );
    QRegExp m_line;
    QRegExp m_lastLine;
    QRegExp m_thisLine;
    QRegExp m_mark;
    QRegExp m_forwardSearch;
    QRegExp m_forwardSearch2;
    QRegExp m_backwardSearch;
    QRegExp m_backwardSearch2;
    QRegExp m_base;
    QRegExp m_offset;
    QRegExp m_position;
    QRegExp m_cmdRange;
  } rangeExpressionParser;

  QString withCaseSensitivityMarkersStripped(const QString& originalSearchTerm)
  {
    // Only \C is handled, for now - I'll implement \c if someone asks for it.
    int pos = 0;
    QString caseSensitivityMarkersStripped = originalSearchTerm;
    while (pos < caseSensitivityMarkersStripped.length())
    {
      if (caseSensitivityMarkersStripped.at(pos) == 'C' && isCharEscaped(caseSensitivityMarkersStripped, pos))
      {
        caseSensitivityMarkersStripped.replace(pos - 1, 2, "");
        pos--;
      }
      pos++;
    }
    return caseSensitivityMarkersStripped;
  }
}

KateViEmulatedCommandBar::KateViEmulatedCommandBar(KateView* view, QWidget* parent)
    : KateViewBarWidget(false, parent),
      m_isActive(false),
      m_mode(NoMode),
      m_view(view),
      m_wasAborted(true),
      m_suspendEditEventFiltering(false),
      m_waitingForRegister(false),
      m_commandResponseMessageTimeOutMS(4000),
      m_nextTextChangeDueToCompletionChange(false),
      m_currentCompletionType(None),
      m_currentSearchIsCaseSensitive(false),
      m_currentSearchIsBackwards(false)
{
  QHBoxLayout * layout = new QHBoxLayout();
  centralWidget()->setLayout(layout);
  m_barTypeIndicator = new QLabel(this);
  m_barTypeIndicator->setObjectName("bartypeindicator");

  m_edit = new QLineEdit(this);
  m_edit->setObjectName("commandtext");
  layout->addWidget(m_edit);

  m_commandResponseMessageDisplay = new QLabel(this);
  m_commandResponseMessageDisplay->setObjectName("commandresponsemessage");
  layout->addWidget(m_commandResponseMessageDisplay);

  m_waitingForRegisterIndicator = new QLabel(this);
  m_waitingForRegisterIndicator->setObjectName("waitingforregisterindicator");
  m_waitingForRegisterIndicator->setVisible(false);
  m_waitingForRegisterIndicator->setText("\"");
  layout->addWidget(m_waitingForRegisterIndicator);

  updateMatchHighlightAttrib();
  m_highlightedMatch = m_view->doc()->newMovingRange(Range(), Kate::TextRange::DoNotExpand);
  m_highlightedMatch->setView(m_view); // show only in this view
  m_highlightedMatch->setAttributeOnlyForViews(true);
  // use z depth defined in moving ranges interface
  m_highlightedMatch->setZDepth (-10000.0);
  m_highlightedMatch->setAttribute(m_highlightMatchAttribute);

  m_edit->installEventFilter(this);
  connect(m_edit, SIGNAL(textChanged(QString)), this, SLOT(editTextChanged(QString)));
  connect(m_view, SIGNAL(configChanged()),
          this, SLOT(updateMatchHighlightAttrib()));

  m_completer = new QCompleter(QStringList() << "foo", m_edit);
  // Can't find a way to stop the QCompleter from auto-completing when attached to a QLineEdit,
  // so don't actually set it as the QLineEdit's completer.
  m_completer->setWidget(m_edit);
  m_completer->setObjectName("completer");
  m_completionModel = new QStringListModel;
  m_completer->setModel(m_completionModel);
  m_completer->setCaseSensitivity(Qt::CaseInsensitive);
  m_completer->popup()->installEventFilter(this);

  m_commandResponseMessageDisplayHide = new QTimer(this);
  m_commandResponseMessageDisplayHide->setSingleShot(true);
  connect(m_commandResponseMessageDisplayHide, SIGNAL(timeout()),
          this, SIGNAL(hideMe()));
  // make sure the timer is stopped when the user switches views. if not, focus will be given to the
  // wrong view when KateViewBar::hideCurrentBarWidget() is called as a result of m_commandResponseMessageDisplayHide
  // timing out.
  connect(m_view, SIGNAL(focusOut(KTextEditor::View*)), m_commandResponseMessageDisplayHide, SLOT(stop()));
  // We can restart the timer once the view has focus again, though.
  connect(m_view, SIGNAL(focusIn(KTextEditor::View*)), this, SLOT(startHideCommandResponseTimer()));
}

KateViEmulatedCommandBar::~KateViEmulatedCommandBar()
{
  delete m_highlightedMatch;
}

void KateViEmulatedCommandBar::init(KateViEmulatedCommandBar::Mode mode, const QString& initialText)
{
  m_currentCompletionType = None;
  QChar barTypeIndicator = QChar::Null;
  switch(mode)
  {
    case SearchForward:
      barTypeIndicator = '/';
      break;
    case SearchBackward:
      barTypeIndicator = '?';
      break;
    case Command:
      barTypeIndicator = ':';
      break;
    default:
      Q_ASSERT(false && "Unknown mode!");
  }
  m_barTypeIndicator->setText(barTypeIndicator);
  setBarBackground(Normal);

  m_startingCursorPos = m_view->cursorPosition();

  m_mode = mode;
  m_edit->setFocus();
  m_edit->setText(initialText);
  m_edit->show();

  m_commandResponseMessageDisplay->hide();
  m_commandResponseMessageDisplayHide->stop();;

  m_isActive = true;

  m_wasAborted = true;

  // A change in focus will have occurred: make sure we process it now, instead of having it
  // occur later and stop() m_commandResponseMessageDisplayHide.
  // This is generally only a problem when feeding a sequence of keys without human intervention,
  // as when we execute a mapping, macro, or test case.
  while(QApplication::hasPendingEvents())
  {
    QApplication::processEvents();
  }
}

bool KateViEmulatedCommandBar::isActive()
{
  return m_isActive;
}

void KateViEmulatedCommandBar::setCommandResponseMessageTimeout(long int commandResponseMessageTimeOutMS)
{
  m_commandResponseMessageTimeOutMS = commandResponseMessageTimeOutMS;
}

void KateViEmulatedCommandBar::closed()
{
  // Close can be called multiple times between init()'s, so only reset the cursor once!
  if (m_startingCursorPos.isValid())
  {
    if (m_wasAborted)
    {
      moveCursorTo(m_startingCursorPos);
    }
  }
  m_startingCursorPos = KTextEditor::Cursor::invalid();
  updateMatchHighlight(Range::invalid());
  m_completer->popup()->hide();
  m_isActive = false;

  if (m_mode == SearchForward || m_mode == SearchBackward)
  {
    // Send a synthetic keypress through the system that signals whether the search was aborted or
    // not.  If not, the keypress will "complete" the search motion, thus triggering it.
    const Qt::Key syntheticSearchCompletedKey = (m_wasAborted ? static_cast<Qt::Key>(0) : Qt::Key_Enter);
    QKeyEvent syntheticSearchCompletedKeyPress(QEvent::KeyPress, syntheticSearchCompletedKey, Qt::NoModifier);
    m_view->getViInputModeManager()->handleKeypress(&syntheticSearchCompletedKeyPress);
    if (!m_wasAborted)
    {
      m_view->getViInputModeManager()->setLastSearchPattern(m_currentSearchPattern);
      m_view->getViInputModeManager()->setLastSearchCaseSensitive(m_currentSearchIsCaseSensitive);
      m_view->getViInputModeManager()->setLastSearchBackwards(m_currentSearchIsBackwards);
    }
    KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem(m_edit->text());
  }
  else
  {
    if (m_wasAborted)
    {
      // Appending the command to the history when it is executed is handled elsewhere; we can't
      // do it inside closed() as we may still be showing the command response display.
      KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem(m_edit->text());
      // With Vim, aborting a command returns us to Normal mode, even if we were in Visual Mode.
      // If we switch from Visual to Normal mode, we need to clear the selection.
      m_view->clearSelection();
    }
  }
}

void KateViEmulatedCommandBar::updateMatchHighlightAttrib()
{
  const QColor& matchColour = m_view->renderer()->config()->searchHighlightColor();
  if (!m_highlightMatchAttribute)
  {
    m_highlightMatchAttribute = new KTextEditor::Attribute;
  }
  m_highlightMatchAttribute->setBackground(matchColour);
  KTextEditor::Attribute::Ptr mouseInAttribute(new KTextEditor::Attribute());
  m_highlightMatchAttribute->setDynamicAttribute (KTextEditor::Attribute::ActivateMouseIn, mouseInAttribute);
  m_highlightMatchAttribute->dynamicAttribute (KTextEditor::Attribute::ActivateMouseIn)->setBackground(matchColour);
}

void KateViEmulatedCommandBar::updateMatchHighlight(const Range& matchRange)
{
  // Note that if matchRange is invalid, the highlight will not be shown, so we
  // don't need to check for that explicitly.
  m_highlightedMatch->setRange(matchRange);
}

void KateViEmulatedCommandBar::setBarBackground(KateViEmulatedCommandBar::BarBackgroundStatus status)
{
    QPalette barBackground(m_edit->palette());
    switch (status)
    {
      case MatchFound:
      {
        KColorScheme::adjustBackground(barBackground, KColorScheme::PositiveBackground);
        break;
      }
      case NoMatchFound:
      {
        KColorScheme::adjustBackground(barBackground, KColorScheme::NegativeBackground);
        break;
      }
      case Normal:
      {
        barBackground = QPalette();
        break;
      }
    }
    m_edit->setPalette(barBackground);
}

bool KateViEmulatedCommandBar::eventFilter(QObject* object, QEvent* event)
{
  Q_ASSERT(object == m_edit || object == m_completer->popup());
  if (m_suspendEditEventFiltering)
  {
    return false;
  }
  Q_UNUSED(object);
  if (event->type() == QEvent::KeyPress)
  {
    return m_view->getViInputModeManager()->handleKeypress(static_cast<QKeyEvent*>(event));
  }
  return false;
}

void KateViEmulatedCommandBar::deleteSpacesToLeftOfCursor()
{
  while (m_edit->cursorPosition() != 0 && m_edit->text()[m_edit->cursorPosition() - 1] == ' ')
  {
    m_edit->backspace();
  }
}

void KateViEmulatedCommandBar::deleteWordCharsToLeftOfCursor()
{
  while (m_edit->cursorPosition() != 0)
  {
    const QChar charToTheLeftOfCursor = m_edit->text()[m_edit->cursorPosition() - 1];
    if (!charToTheLeftOfCursor.isLetterOrNumber() && charToTheLeftOfCursor != '_')
    {
      break;
    }

    m_edit->backspace();
  }
}

bool KateViEmulatedCommandBar::deleteNonWordCharsToLeftOfCursor()
{
  bool deletionsMade = false;
  while (m_edit->cursorPosition() != 0)
  {
    const QChar charToTheLeftOfCursor = m_edit->text()[m_edit->cursorPosition() - 1];
    if (charToTheLeftOfCursor.isLetterOrNumber() || charToTheLeftOfCursor == '_' || charToTheLeftOfCursor == ' ')
    {
      break;
    }

    m_edit->backspace();
    deletionsMade = true;
  }
  return deletionsMade;
}

QString KateViEmulatedCommandBar::wordBeforeCursor()
{
  int wordBeforeCursorBegin = m_edit->cursorPosition() - 1;
  while (wordBeforeCursorBegin >= 0 && (m_edit->text()[wordBeforeCursorBegin].isLetterOrNumber() || m_edit->text()[wordBeforeCursorBegin] == '_'))
  {
    wordBeforeCursorBegin--;
  }
  wordBeforeCursorBegin++;
  return m_edit->text().mid(wordBeforeCursorBegin, m_edit->cursorPosition() - wordBeforeCursorBegin);
}

QString KateViEmulatedCommandBar::commandBeforeCursor()
{
  const QString textWithoutLeadingRange = withoutLeadingRange();
  const int cursorPositionWithoutLeadingRange = m_edit->cursorPosition() - leadingRange().length();
  int commandBeforeCursorBegin = cursorPositionWithoutLeadingRange - 1;
  while (commandBeforeCursorBegin >= 0 && (textWithoutLeadingRange[commandBeforeCursorBegin].isLetterOrNumber() || textWithoutLeadingRange[commandBeforeCursorBegin] == '_' || textWithoutLeadingRange[commandBeforeCursorBegin] == '-'))
  {
    commandBeforeCursorBegin--;
  }
  commandBeforeCursorBegin++;
  return textWithoutLeadingRange.mid(commandBeforeCursorBegin, cursorPositionWithoutLeadingRange - commandBeforeCursorBegin);

}

void KateViEmulatedCommandBar::replaceWordBeforeCursorWith(const QString& newWord)
{
  const QString newText = m_edit->text().left(m_edit->cursorPosition() - wordBeforeCursor().length()) +
  newWord +
  m_edit->text().mid(m_edit->cursorPosition());
  m_edit->setText(newText);
}

void KateViEmulatedCommandBar::replaceCommandBeforeCursorWith(const QString& newCommand)
{
  const QString newText = m_edit->text().left(m_edit->cursorPosition() - commandBeforeCursor().length()) +
  newCommand +
  m_edit->text().mid(m_edit->cursorPosition());
  m_edit->setText(newText);
}


void KateViEmulatedCommandBar::activateSearchHistoryCompletion()
{
  m_currentCompletionType = SearchHistory;
  m_completionModel->setStringList(reversed(KateGlobal::self()->viInputModeGlobal()->searchHistory()));
  updateCompletionPrefix();
  m_completer->complete();
}

void KateViEmulatedCommandBar::activateWordFromDocumentCompletion()
{
    m_currentCompletionType = WordFromDocument;
    QRegExp wordRegEx("\\w{1,}");
    QStringList foundWords;
    // Narrow the range of lines we search around the cursor so that we don't die on huge files.
    const int startLine = qMax(0, m_view->cursorPosition().line() - 4096);
    const int endLine = qMin(m_view->document()->lines(), m_view->cursorPosition().line() + 4096);
    for (int lineNum = startLine; lineNum < endLine; lineNum++)
    {
      const QString line = m_view->document()->line(lineNum);
      int wordSearchBeginPos = 0;
      while (wordRegEx.indexIn(line, wordSearchBeginPos) != -1)
      {
        const QString foundWord = wordRegEx.cap(0);
        foundWords << foundWord;
        wordSearchBeginPos = wordRegEx.indexIn(line, wordSearchBeginPos) + wordRegEx.matchedLength();
      }
    }
    foundWords = QSet<QString>::fromList(foundWords).toList();
    qSort(foundWords.begin(), foundWords.end(), caseInsensitiveLessThan);
    m_completionModel->setStringList(foundWords);
    updateCompletionPrefix();
    m_completer->complete();
}

void KateViEmulatedCommandBar::activateCommandCompletion()
{
    m_completionModel->setStringList(KateCmd::self()->commandCompletionObject()->items());
    m_currentCompletionType = Commands;
}

void KateViEmulatedCommandBar::activateCommandHistoryCompletion()
{
  m_currentCompletionType = CommandHistory;
  m_completionModel->setStringList(reversed(KateGlobal::self()->viInputModeGlobal()->commandHistory()));
  updateCompletionPrefix();
  m_completer->complete();
}

void KateViEmulatedCommandBar::activateSedSearchHistoryCompletion()
{
  if (!KateGlobal::self()->viInputModeGlobal()->searchHistory().isEmpty())
  {
    m_currentCompletionType = SedSearchHistory;
    m_completionModel->setStringList(reversed(KateGlobal::self()->viInputModeGlobal()->searchHistory()));
    m_completer->setCompletionPrefix(findTermInSedReplace());
    m_completer->complete();
  }
}

void KateViEmulatedCommandBar::activateSedReplaceHistoryCompletion()
{
  if (!KateGlobal::self()->viInputModeGlobal()->replaceHistory().isEmpty())
  {
    m_currentCompletionType = SedReplaceHistory;
    m_completionModel->setStringList(reversed(KateGlobal::self()->viInputModeGlobal()->replaceHistory()));
    m_completer->setCompletionPrefix(replaceTermInSedReplace());
    m_completer->complete();
  }
}

void KateViEmulatedCommandBar::deactivateCompletion()
{
  kDebug(13070) << "Manually dismissing completions";
  m_completer->popup()->hide();
  m_currentCompletionType = None;
}

void KateViEmulatedCommandBar::updateCompletionPrefix()
{
  if (m_currentCompletionType == WordFromDocument)
  {
    m_completer->setCompletionPrefix(wordBeforeCursor());
  }
  else if (m_currentCompletionType == SearchHistory)
  {
    m_completer->setCompletionPrefix(m_edit->text());
  }
  else if (m_currentCompletionType == CommandHistory)
  {
    m_completer->setCompletionPrefix(m_edit->text());
  }
  else if (m_currentCompletionType == Commands)
  {
    m_completer->setCompletionPrefix(commandBeforeCursor());
  }
  else
  {
    Q_ASSERT(false && "Unhandled completion type");
  }
  // Seem to need this to alter the size of the popup box appropriately.
  m_completer->complete();
}

void KateViEmulatedCommandBar::currentCompletionChanged()
{
  const QString newCompletion = m_completer->currentCompletion();
  if (newCompletion.isEmpty())
  {
    return;
  }
  m_nextTextChangeDueToCompletionChange = true;
  if (m_currentCompletionType == WordFromDocument)
  {
    replaceWordBeforeCursorWith(newCompletion);
  }
  else if (m_currentCompletionType == SearchHistory)
  {
    m_edit->setText(newCompletion);
  }
  else if (m_currentCompletionType == CommandHistory)
  {
    m_edit->setText(newCompletion);
  }
  else if (m_currentCompletionType == Commands)
  {
    const int newCursorPosition = m_edit->cursorPosition() + (newCompletion.length() - commandBeforeCursor().length());
    replaceCommandBeforeCursorWith(newCompletion);
    m_edit->setCursorPosition(newCursorPosition);
  }
  else if (m_currentCompletionType == SedSearchHistory)
  {
    ParsedSedReplace parsedSedReplace = parseAsSedReplaceExpression();
    QString delimiterEscaped = ensuredCharEscaped(newCompletion, parsedSedReplace.delimiter);
    m_edit->setText(findTermInSedReplaceReplacedWith(delimiterEscaped));
    parsedSedReplace = parseAsSedReplaceExpression();
    m_edit->setCursorPosition(parsedSedReplace.findEndPos + 1);
  }
  else if (m_currentCompletionType == SedReplaceHistory)
  {
    ParsedSedReplace parsedSedReplace = parseAsSedReplaceExpression();
    QString delimiterEscaped = ensuredCharEscaped(newCompletion, parsedSedReplace.delimiter);
    m_edit->setText(replaceTermInSedReplaceReplacedWith(delimiterEscaped));
    parsedSedReplace = parseAsSedReplaceExpression();
    m_edit->setCursorPosition(parsedSedReplace.replaceEndPos + 1);
  }
  else
  {
    Q_ASSERT(false && "Something went wrong, here - completion with unrecognised completion type");
  }
  m_nextTextChangeDueToCompletionChange = false;
}

void KateViEmulatedCommandBar::setCompletionIndex(int index)
{
  const QModelIndex modelIndex = m_completer->popup()->model()->index(index, 0);
  // Need to set both of these, for some reason.
  m_completer->popup()->setCurrentIndex(modelIndex);
  m_completer->setCurrentRow(index);

  m_completer->popup()->scrollTo(modelIndex);

  currentCompletionChanged();
}

KateViEmulatedCommandBar::ParsedSedReplace KateViEmulatedCommandBar::parseAsSedReplaceExpression()
{
  const QString commandWithoutLeadingRange = withoutLeadingRange();
  ParsedSedReplace parsedSedReplace;
  QString delimiter;
  parsedSedReplace.parsedSuccessfully = KateCommands::SedReplace::parse(commandWithoutLeadingRange, delimiter, parsedSedReplace.findBeginPos, parsedSedReplace.findEndPos, parsedSedReplace.replaceBeginPos, parsedSedReplace.replaceEndPos);
  if (parsedSedReplace.parsedSuccessfully)
  {
    parsedSedReplace.delimiter = delimiter.at(0);
    if (parsedSedReplace.replaceBeginPos == -1)
    {
      if (parsedSedReplace.findBeginPos != -1)
      {
        // The replace term was empty, and a quirk of the regex used is that replaceBeginPos will be -1.
        // It's actually the position after the first occurrence of the delimiter after the end of the find pos.
        parsedSedReplace.replaceBeginPos = commandWithoutLeadingRange.indexOf(delimiter, parsedSedReplace.findEndPos) + 1;
        parsedSedReplace.replaceEndPos = parsedSedReplace.replaceBeginPos - 1;
      }
      else
      {
        // Both find and replace terms are empty; replace term is at the third occurrence of the delimiter.
        parsedSedReplace.replaceBeginPos = 0;
        for (int delimiterCount = 1; delimiterCount <= 3; delimiterCount++)
        {
          parsedSedReplace.replaceBeginPos = commandWithoutLeadingRange.indexOf(delimiter, parsedSedReplace.replaceBeginPos + 1);
        }
        parsedSedReplace.replaceEndPos = parsedSedReplace.replaceBeginPos - 1;
      }
    }
    if (parsedSedReplace.findBeginPos == -1)
    {
      // The find term was empty, and a quirk of the regex used is that findBeginPos will be -1.
      // It's actually the position after the first occurrence of the delimiter.
      parsedSedReplace.findBeginPos = commandWithoutLeadingRange.indexOf(delimiter) + 1;
      parsedSedReplace.findEndPos = parsedSedReplace.findBeginPos - 1;
    }

  }

  if (parsedSedReplace.parsedSuccessfully)
  {
    parsedSedReplace.findBeginPos += leadingRange().length();
    parsedSedReplace.findEndPos += leadingRange().length();
    parsedSedReplace.replaceBeginPos += leadingRange().length();
    parsedSedReplace.replaceEndPos += leadingRange().length();
  }
  return parsedSedReplace;
}

QString KateViEmulatedCommandBar::findTermInSedReplaceReplacedWith(const QString& newFindTerm)
{
  const QString command = m_edit->text();
  ParsedSedReplace parsedSedReplace = parseAsSedReplaceExpression();
  Q_ASSERT(parsedSedReplace.parsedSuccessfully);
  return command.mid(0, parsedSedReplace.findBeginPos) +
    newFindTerm +
    command.mid(parsedSedReplace.findEndPos + 1);

}

QString KateViEmulatedCommandBar::replaceTermInSedReplaceReplacedWith(const QString& newReplaceTerm)
{
  const QString command = m_edit->text();
  ParsedSedReplace parsedSedReplace = parseAsSedReplaceExpression();
  Q_ASSERT(parsedSedReplace.parsedSuccessfully);
  return command.mid(0, parsedSedReplace.replaceBeginPos) +
    newReplaceTerm +
    command.mid(parsedSedReplace.replaceEndPos + 1);
}

QString KateViEmulatedCommandBar::findTermInSedReplace()
{
  const QString command = m_edit->text();
  ParsedSedReplace parsedSedReplace = parseAsSedReplaceExpression();
  Q_ASSERT(parsedSedReplace.parsedSuccessfully);
  return command.mid(parsedSedReplace.findBeginPos, parsedSedReplace.findEndPos - parsedSedReplace.findBeginPos + 1);
}

QString KateViEmulatedCommandBar::replaceTermInSedReplace()
{
  const QString command = m_edit->text();
  ParsedSedReplace parsedSedReplace = parseAsSedReplaceExpression();
  Q_ASSERT(parsedSedReplace.parsedSuccessfully);
  return command.mid(parsedSedReplace.replaceBeginPos, parsedSedReplace.replaceEndPos - parsedSedReplace.replaceBeginPos + 1);
}

bool KateViEmulatedCommandBar::isCursorInFindTermOfSedReplace()
{
  ParsedSedReplace parsedSedReplace = parseAsSedReplaceExpression();
  return parsedSedReplace.parsedSuccessfully && (m_edit->cursorPosition() >= parsedSedReplace.findBeginPos && m_edit->cursorPosition() <= parsedSedReplace.findEndPos + 1);
}

bool KateViEmulatedCommandBar::isCursorInReplaceTermOfSedReplace()
{
  ParsedSedReplace parsedSedReplace = parseAsSedReplaceExpression();
  return parsedSedReplace.parsedSuccessfully && m_edit->cursorPosition() >= parsedSedReplace.replaceBeginPos && m_edit->cursorPosition() <= parsedSedReplace.replaceEndPos + 1;
}

QString KateViEmulatedCommandBar::withoutLeadingRange()
{
  const QString originalCommand = m_edit->text();
  return originalCommand.mid(leadingRange().length());
}

QString KateViEmulatedCommandBar::leadingRange()
{
  QString leadingRange;
  QString unused;
  const QString command = m_edit->text();
  parseRangeExpression(command, m_view, leadingRange, unused);
  return leadingRange;
}

bool KateViEmulatedCommandBar::handleKeyPress(const QKeyEvent* keyEvent)
{
  if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_Space)
  {
    activateWordFromDocumentCompletion();
    return true;
  }
  if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_P)
  {
    if (!m_completer->popup()->isVisible())
    {
      if (m_mode == Command)
      {
        if (isCursorInFindTermOfSedReplace())
        {
          activateSedSearchHistoryCompletion();
        }
        else if (isCursorInReplaceTermOfSedReplace())
        {
          activateSedReplaceHistoryCompletion();
        }
        else
        {
          activateCommandHistoryCompletion();
        }
      }
      else
      {
        activateSearchHistoryCompletion();
      }
      if (m_currentCompletionType != None)
      {
        setCompletionIndex(0);
      }
    }
    else
    {
      // Descend to next row, wrapping around if necessary.
      if (m_completer->currentRow() + 1 == m_completer->completionCount())
      {
        setCompletionIndex(0);
      }
      else
      {
        setCompletionIndex(m_completer->currentRow() + 1);
      }
    }
    return true;
  }
  if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_N)
  {
    if (!m_completer->popup()->isVisible())
    {
      if (m_mode == Command)
      {
        activateCommandHistoryCompletion();
      }
      else
      {
        activateSearchHistoryCompletion();
      }
      setCompletionIndex(m_completer->completionCount() - 1);
    }
    else
    {
      // Ascend to previous row, wrapping around if necessary.
      if (m_completer->currentRow() == 0)
      {
        setCompletionIndex(m_completer->completionCount() - 1);
      }
      else
      {
        setCompletionIndex(m_completer->currentRow() - 1);
      }
    }
    return true;
  }
  if (m_waitingForRegister)
  {
    if (keyEvent->key() != Qt::Key_Shift && keyEvent->key() != Qt::Key_Control)
    {
      const QChar key = KateViKeyParser::self()->KeyEventToQChar(
                  keyEvent->key(),
                  keyEvent->text(),
                  keyEvent->modifiers(),
                  keyEvent->nativeScanCode() ).toLower();

      const int oldCursorPosition = m_edit->cursorPosition();
      QString textToInsert;
      if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_W)
      {
        textToInsert = m_view->doc()->getWord(m_view->cursorPosition());
      }
      else
      {
        textToInsert = KateGlobal::self()->viInputModeGlobal()->getRegisterContent( key );
      }
      m_edit->setText(m_edit->text().insert(m_edit->cursorPosition(), textToInsert));
      m_edit->setCursorPosition(oldCursorPosition + textToInsert.length());
      m_waitingForRegister = false;
      m_waitingForRegisterIndicator->setVisible(false);
    }
  } else if (keyEvent->modifiers() == Qt::ControlModifier)
  {
    if (keyEvent->key() == Qt::Key_C || keyEvent->key() == Qt::Key_BracketLeft)
    {
      if (m_currentCompletionType == None || !m_completer->popup()->isVisible())
      {
        emit hideMe();
      }
      else
      {
        deactivateCompletion();
        m_nextTextChangeDueToCompletionChange = true;
        m_edit->setText(m_revertToIfCompletionAborted);
        m_edit->setCursorPosition(m_revertToCursorPosIfCompletionAborted);
        m_nextTextChangeDueToCompletionChange = false;
      }
      return true;
    }
    else if (keyEvent->key() == Qt::Key_H)
    {
      if (m_edit->text().isEmpty())
      {
        emit hideMe();
      }
      m_edit->backspace();
      return true;
    }
    else if (keyEvent->key() == Qt::Key_B)
    {
      m_edit->setCursorPosition(0);
      return true;
    }
    else if (keyEvent->key() == Qt::Key_W)
    {
      deleteSpacesToLeftOfCursor();
      if(!deleteNonWordCharsToLeftOfCursor())
      {
        deleteWordCharsToLeftOfCursor();
      }
      return true;
    }
    else if (keyEvent->key() == Qt::Key_R)
    {
      m_waitingForRegister = true;
      m_waitingForRegisterIndicator->setVisible(true);
      return true;
    }
    else if (keyEvent->key() == Qt::Key_D || keyEvent->key() == Qt::Key_F)
    {
      if (m_mode == Command)
      {
        ParsedSedReplace parsedSedReplace = parseAsSedReplaceExpression();
        if (parsedSedReplace.parsedSuccessfully)
        {
          const bool replaceFindTerm = (keyEvent->key() == Qt::Key_D);
          const QString textWithTermCleared =  (replaceFindTerm) ?
                findTermInSedReplaceReplacedWith("") :
                replaceTermInSedReplaceReplacedWith("");
          m_edit->setText(textWithTermCleared);
          ParsedSedReplace parsedSedReplaceAfter = parseAsSedReplaceExpression();
          const int newCursorPos = replaceFindTerm ? parsedSedReplaceAfter.findBeginPos :
                                                     parsedSedReplaceAfter.replaceBeginPos;
          m_edit->setCursorPosition(newCursorPos);
        }
      }
      return true;
    }
    return false;
  }
  else if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
  {
    if (m_completer->popup()->isVisible() && m_currentCompletionType == KateViEmulatedCommandBar::WordFromDocument)
    {
      deactivateCompletion();
    }
    else
    {
      m_wasAborted = false;
      deactivateCompletion();
      if (m_mode == Command)
      {
        kDebug(13070) << "Executing: " << m_edit->text();
        QString commandToExecute = m_edit->text();
        ParsedSedReplace parsedSedReplace = parseAsSedReplaceExpression();
        qDebug() << "text:\n" << m_edit->text() << "\n is sed replace: " << parsedSedReplace.parsedSuccessfully;
        if (parsedSedReplace.parsedSuccessfully)
        {
          const QString originalFindTerm = findTermInSedReplace();
          const QString convertedFindTerm = vimRegexToQtRegexPattern(originalFindTerm);
          const QString commandWithSedSearchRegexConverted = findTermInSedReplaceReplacedWith(convertedFindTerm);
          KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem(originalFindTerm);
          const QString replaceTerm = replaceTermInSedReplace();
          KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem(replaceTerm);
          commandToExecute = commandWithSedSearchRegexConverted;
          kDebug(13070) << "Command to execute after replacing search term: "<< commandToExecute;
        }

        // TODO - this is a hack-ish way of finding the response from the command; maybe
        // add another overload of "execute" to KateCommandLineBar that returns the
        // response message ... ?
        m_view->cmdLineBar()->setText("");
        m_view->cmdLineBar()->execute(commandToExecute);
        KateCmdLineEdit *kateCommandLineEdit = m_view->cmdLineBar()->findChild<KateCmdLineEdit*>();
        Q_ASSERT(kateCommandLineEdit);
        const QString commandResponseMessage = kateCommandLineEdit->text();
        if (commandResponseMessage.isEmpty())
        {
          emit hideMe();
        }
        else
        {
          // Display the message for a while.  Become inactive, so we don't steal keys in the meantime.
          m_isActive = false;
          m_edit->hide();
          m_commandResponseMessageDisplay->show();
          m_commandResponseMessageDisplay->setText(commandResponseMessage);
          m_commandResponseMessageDisplayHide->start(m_commandResponseMessageTimeOutMS);
        }
        KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem(m_edit->text());
      }
      else
      {
        emit hideMe();
      }
    }
    return true;
  }
  else
  {
    m_suspendEditEventFiltering = true;
    QKeyEvent keyEventCopy(keyEvent->type(), keyEvent->key(), keyEvent->modifiers(), keyEvent->text(), keyEvent->isAutoRepeat(), keyEvent->count());
    qApp->notify(m_edit, &keyEventCopy);
    m_suspendEditEventFiltering = false;
  }
  return true;
}

Range KateViEmulatedCommandBar::parseRangeExpression(const QString& command, KateView *view, QString& destRangeExpression, QString& destTransformedCommand)
{
  return rangeExpressionParser.parseRangeExpression(command, destRangeExpression, destTransformedCommand, view);
}

void KateViEmulatedCommandBar::moveCursorTo(const Cursor& cursorPos)
{
  m_view->setCursorPosition(cursorPos);
  if (m_view->getCurrentViMode() == VisualMode || m_view->getCurrentViMode() == VisualLineMode)
  {
    m_view->getViInputModeManager()->getViVisualMode()->goToPos(cursorPos);
  }
}

void KateViEmulatedCommandBar::editTextChanged(const QString& newText)
{
  qDebug() << "New text: " << newText;
  if (!m_nextTextChangeDueToCompletionChange)
  {
    m_revertToIfCompletionAborted = newText;
    m_revertToCursorPosIfCompletionAborted = m_edit->cursorPosition();
  }
  if (m_mode == SearchForward || m_mode == SearchBackward)
  {
    QString qtRegexPattern = (newText == "/") ? m_view->getViInputModeManager()->getLastSearchPattern() : vimRegexToQtRegexPattern(newText);

    // Decide case-sensitivity via SmartCase (note: if the expression contains \C, the "case-sensitive" marker, then
    // we will be case-sensitive "by coincidence", as it were.).
    bool caseSensitive = true;
    if (qtRegexPattern.toLower() == qtRegexPattern)
    {
      caseSensitive = false;
    }

    qtRegexPattern = withCaseSensitivityMarkersStripped(qtRegexPattern);

    qDebug() << "Final regex: " << qtRegexPattern;

    const bool searchBackwards = (m_mode == SearchBackward);

    m_currentSearchPattern = qtRegexPattern;
    m_currentSearchIsCaseSensitive = caseSensitive;
    m_currentSearchIsBackwards = searchBackwards;

    // The "count" for the current search is not shared between Visual & Normal mode, so we need to pick
    // the right one to handle the counted search.
    Range match = m_view->getViInputModeManager()->getCurrentViModeHandler()->findPattern(qtRegexPattern, searchBackwards, caseSensitive, m_startingCursorPos);

    if (match.isValid())
    {
      moveCursorTo(match.start());
      setBarBackground(MatchFound);
    }
    else
    {
      moveCursorTo(m_startingCursorPos);
      if (!m_edit->text().isEmpty())
      {
        setBarBackground(NoMatchFound);
      }
      else
      {
        setBarBackground(Normal);
      }
    }

    updateMatchHighlight(match);

  }

  // Command completion doesn't need to be manually invoked.
  if (m_mode == Command && m_currentCompletionType == None && !withoutLeadingRange().isEmpty())
  {
    activateCommandCompletion();
  }

  // Command completion mode should be automatically invoked if we are in Command mode, but
  // only if this is the leading word in the text edit (it gets annoying if completion pops up
  // after ":s/se" etc).
  const bool commandBeforeCursorIsLeading = (m_edit->cursorPosition() - commandBeforeCursor().length() == leadingRange().length());
  if (m_mode == Command && !commandBeforeCursorIsLeading && m_currentCompletionType == Commands && !m_nextTextChangeDueToCompletionChange)
  {
    deactivateCompletion();
  }

  // If we edit the text after having selected a completion, this means we implicitly accept it,
  // and so we should dismiss it.
  if (!m_nextTextChangeDueToCompletionChange && m_completer->popup()->currentIndex().row() != -1)
  {
    deactivateCompletion();
  }

  if (m_currentCompletionType != None && !m_nextTextChangeDueToCompletionChange)
  {
    updateCompletionPrefix();
  }

}

void KateViEmulatedCommandBar::startHideCommandResponseTimer()
{
  if (m_commandResponseMessageDisplay->isVisible() && !m_commandResponseMessageDisplayHide->isActive())
  {
    m_commandResponseMessageDisplayHide->start(m_commandResponseMessageTimeOutMS);
  }
}


RangeExpressionParser::RangeExpressionParser()
{
  m_line.setPattern("\\d+");
  m_lastLine.setPattern("\\$");
  m_thisLine.setPattern("\\.");
  m_mark.setPattern("\\'[0-9a-z><\\+\\*\\_]");
  m_forwardSearch.setPattern("/([^/]*)/?");
  m_forwardSearch2.setPattern("/[^/]*/?"); // no group
  m_backwardSearch.setPattern("\\?([^?]*)\\??");
  m_backwardSearch2.setPattern("\\?[^?]*\\??"); // no group
  m_base.setPattern("(?:" + m_mark.pattern() + ")|(?:" +
                        m_line.pattern() + ")|(?:" +
                        m_thisLine.pattern() + ")|(?:" +
                        m_lastLine.pattern() + ")|(?:" +
                        m_forwardSearch2.pattern() + ")|(?:" +
                        m_backwardSearch2.pattern() + ")");
  m_offset.setPattern("[+-](?:" + m_base.pattern() + ")?");

  // The position regexp contains two groups: the base and the offset.
  // The offset may be empty.
  m_position.setPattern("(" + m_base.pattern() + ")((?:" + m_offset.pattern() + ")*)");

  // The range regexp contains seven groups: the first is the start position, the second is
  // the base of the start position, the third is the offset of the start position, the
  // fourth is the end position including a leading comma, the fifth is end position
  // without the comma, the sixth is the base of the end position, and the seventh is the
  // offset of the end position. The third and fourth groups may be empty, and the
  // fifth, sixth and seventh groups are contingent on the fourth group.
  m_cmdRange.setPattern("^(" + m_position.pattern() + ")((?:,(" + m_position.pattern() + "))?)");
}

Range RangeExpressionParser::parseRangeExpression(const QString& command, QString& destRangeExpression, QString& destTransformedCommand, KateView *view)
{
  Range parsedRange(0, -1, 0, -1);
  if (command.isEmpty())
  {
    return parsedRange;
  }
  QString commandTmp = command;
  bool leadingRangeWasPercent = false;
  // expand '%' to '1,$' ("all lines") if at the start of the line
  if ( commandTmp.at( 0 ) == '%' ) {
    commandTmp.replace( 0, 1, "1,$" );
    leadingRangeWasPercent = true;
  }
  if (m_cmdRange.indexIn(commandTmp) != -1 && m_cmdRange.matchedLength() > 0) {
    commandTmp.remove(m_cmdRange);

    QString position_string1 = m_cmdRange.capturedTexts().at(1);
    QString position_string2 = m_cmdRange.capturedTexts().at(4);
    int position1 = calculatePosition(position_string1, view);

    int position2;
    if (!position_string2.isEmpty()) {
      // remove the comma
      position_string2 = m_cmdRange.capturedTexts().at(5);
      position2 = calculatePosition(position_string2, view);
    } else {
      position2 = position1;
    }

    // special case: if the command is just a number with an optional +/- prefix, rewrite to "goto"
    if (commandTmp.isEmpty()) {
      commandTmp = QString("goto %1").arg(position1);
    } else {
      parsedRange.setRange(KTextEditor::Range(position1 - 1, 0, position2 - 1, 0));
    }

    destRangeExpression = (leadingRangeWasPercent ? "%" : m_cmdRange.cap(0));
    destTransformedCommand = commandTmp;
  }

  return parsedRange;
}

int RangeExpressionParser::calculatePosition(const QString& string, KateView* view ) {

  int pos = 0;
  QList<bool> operators_list;
  QStringList split = string.split(QRegExp("[-+](?!([+-]|$))"));
  QList<int> values;

  foreach ( QString line, split ) {
    pos += line.size();

    if ( pos < string.size() ) {
      if ( string.at(pos) == '+' ) {
        operators_list.push_back( true );
      } else if ( string.at(pos) == '-' ) {
        operators_list.push_back( false );
      } else {
        Q_ASSERT( false );
      }
    }

    ++pos;

    if ( m_line.exactMatch(line) ) {
      values.push_back( line.toInt() );
    } else if ( m_lastLine.exactMatch(line) ) {
      values.push_back( view->doc()->lines() );
    } else if ( m_thisLine.exactMatch(line) ) {
      values.push_back( view->cursorPosition().line() + 1 );
    } else if ( m_mark.exactMatch(line) ) {
      values.push_back( view->getViInputModeManager()->getMarkPosition(line.at(1)).line() + 1 );
    } else if ( m_forwardSearch.exactMatch(line) ) {
      m_forwardSearch.indexIn(line);
      QString pattern = m_forwardSearch.capturedTexts().at(1);
      int match = view->doc()->searchText( Range( view->cursorPosition(), view->doc()->documentEnd() ),
                                             pattern, KTextEditor::Search::Regex ).first().start().line();
      values.push_back( (match < 0) ? -1 : match + 1 );
    } else if ( m_backwardSearch.exactMatch(line) ) {
      m_backwardSearch.indexIn(line);
      QString pattern = m_backwardSearch.capturedTexts().at(1);
      int match = view->doc()->searchText( Range( Cursor( 0, 0), view->cursorPosition() ),
                                             pattern, KTextEditor::Search::Regex ).first().start().line();
      values.push_back( (match < 0) ? -1 : match + 1 );
    }
  }

  if (values.isEmpty())
  {
    return -1;
  }

  int result = values.at(0);
  for (int i = 0; i < operators_list.size(); ++i) {
    if ( operators_list.at(i) == true ) {
      result += values.at(i + 1);
    } else {
      result -= values.at(i + 1);
    }
  }

  return result;
}