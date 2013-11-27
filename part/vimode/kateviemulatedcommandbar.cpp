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
#include "katevicommandrangeexpressionparser.h"
#include "kateglobal.h"
#include "kateconfig.h"
#include "katecmd.h"
#include <katecmds.h>

#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QCompleter>
#include <QtGui/QApplication>
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
      const int indexOfEscapeChar = toggledEscapedString.indexOf(escapeChar , searchFrom);
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
    qtRegexPattern = ensuredCharEscaped(qtRegexPattern, '?');
    {
      // All curly brackets, except the closing curly bracket of a matching pair where the opening bracket is escaped,
      // must have their escaping toggled.
      bool lookingForMatchingCloseBracket = false;
      QList<int> matchingClosedCurlyBracketPositions;
      for (int i = 0; i < qtRegexPattern.length(); i++)
      {
        if (qtRegexPattern[i] == '{' && isCharEscaped(qtRegexPattern, i))
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

  /**
   * @return \a originalRegex but escaped in such a way that a Qt regex search for
   * the resulting string will match the string \a originalRegex.
   */
  QString escapedForSearchingAsLiteral(const QString& originalQtRegex)
  {
    QString escapedForSearchingAsLiteral = originalQtRegex;
    escapedForSearchingAsLiteral.replace('\\', "\\\\");
    escapedForSearchingAsLiteral.replace('$', "\\$");
    escapedForSearchingAsLiteral.replace('^', "\\^");
    escapedForSearchingAsLiteral.replace('.', "\\.");
    escapedForSearchingAsLiteral.replace('*', "\\*");
    escapedForSearchingAsLiteral.replace('/', "\\/");
    escapedForSearchingAsLiteral.replace('[', "\\[");
    escapedForSearchingAsLiteral.replace(']', "\\]");
    escapedForSearchingAsLiteral.replace('\n', "\\n");
    return escapedForSearchingAsLiteral;
  }

  QStringList reversed(const QStringList& originalList)
  {
    QStringList reversedList = originalList;
    std::reverse(reversedList.begin(), reversedList.end());
    return reversedList;
  }


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

  int findPosOfSearchConfigMarker(const QString& searchText, const bool isSearchBackwards)
  {
    const QChar searchConfigMarkerChar = (isSearchBackwards ? '?' : '/');
    for (int pos = 0; pos < searchText.length(); pos++)
    {
      if (searchText.at(pos) == searchConfigMarkerChar)
      {
        if (!isCharEscaped(searchText, pos))
        {
          return pos;
        }
      }
    }
    return -1;
  }

  bool isRepeatLastSearch(const QString& searchText, const bool isSearchBackwards)
  {
    const int posOfSearchConfigMarker = findPosOfSearchConfigMarker(searchText, isSearchBackwards);
    if (posOfSearchConfigMarker != -1)
    {
      if (searchText.left(posOfSearchConfigMarker).isEmpty())
      {
        return true;
      }
    }
    return false;
  }

  bool shouldPlaceCursorAtEndOfMatch(const QString& searchText, const bool isSearchBackwards)
  {
    const int posOfSearchConfigMarker = findPosOfSearchConfigMarker(searchText, isSearchBackwards);
    if (posOfSearchConfigMarker != -1)
    {
      if (searchText.length() > posOfSearchConfigMarker + 1 && searchText.at(posOfSearchConfigMarker + 1) == 'e')
      {
        return true;
      }
    }
    return false;
  }

  QString withSearchConfigRemoved(const QString& originalSearchText, const bool isSearchBackwards)
  {
    const int posOfSearchConfigMarker = findPosOfSearchConfigMarker(originalSearchText, isSearchBackwards);
    if (posOfSearchConfigMarker == -1)
    {
      return originalSearchText;
    }
    else
    {
      return originalSearchText.left(posOfSearchConfigMarker);
    }
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
      m_insertedTextShouldBeEscapedForSearchingAsLiteral(false),
      m_commandResponseMessageTimeOutMS(4000),
      m_isNextTextChangeDueToCompletionChange(false),
      m_currentCompletionType(None),
      m_currentSearchIsCaseSensitive(false),
      m_currentSearchIsBackwards(false),
      m_currentSearchPlacesCursorAtEndOfMatch(false),
      m_isSendingSyntheticSearchCompletedKeypress(false)
{
  QHBoxLayout * layout = new QHBoxLayout();
  centralWidget()->setLayout(layout);
  m_barTypeIndicator = new QLabel(this);
  m_barTypeIndicator->setObjectName("bartypeindicator");
  layout->addWidget(m_barTypeIndicator);

  m_edit = new QLineEdit(this);
  m_edit->setObjectName("commandtext");
  layout->addWidget(m_edit);

  m_commandResponseMessageDisplay = new QLabel(this);
  m_commandResponseMessageDisplay->setObjectName("commandresponsemessage");
  m_commandResponseMessageDisplay->setAlignment(Qt::AlignLeft);
  layout->addWidget(m_commandResponseMessageDisplay);

  m_waitingForRegisterIndicator = new QLabel(this);
  m_waitingForRegisterIndicator->setObjectName("waitingforregisterindicator");
  m_waitingForRegisterIndicator->setVisible(false);
  m_waitingForRegisterIndicator->setText("\"");
  layout->addWidget(m_waitingForRegisterIndicator);

  m_interactiveSedReplaceLabel = new QLabel(this);
  m_interactiveSedReplaceLabel->setObjectName("interactivesedreplace");
  m_interactiveSedReplaceActive = false;
  layout->addWidget(m_interactiveSedReplaceLabel);

  updateMatchHighlightAttrib();
  m_highlightedMatch = m_view->doc()->newMovingRange(Range::invalid(), Kate::TextRange::DoNotExpand);
  m_highlightedMatch->setView(m_view); // Show only in this view.
  m_highlightedMatch->setAttributeOnlyForViews(true);
  // Use z depth defined in moving ranges interface.
  m_highlightedMatch->setZDepth (-10000.0);
  m_highlightedMatch->setAttribute(m_highlightMatchAttribute);
  connect(m_view, SIGNAL(configChanged()),
          this, SLOT(updateMatchHighlightAttrib()));

  m_edit->installEventFilter(this);
  connect(m_edit, SIGNAL(textChanged(QString)), this, SLOT(editTextChanged(QString)));

  m_completer = new QCompleter(QStringList(), m_edit);
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
  // Make sure the timer is stopped when the user switches views. If not, focus will be given to the
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
  m_mode = mode;
  m_isActive = true;
  m_wasAborted = true;

  showBarTypeIndicator(mode);

  setBarBackground(Normal);

  m_startingCursorPos = m_view->cursorPosition();

  m_interactiveSedReplaceActive = false;
  m_interactiveSedReplaceLabel->hide();

  m_edit->setFocus();
  m_edit->setText(initialText);
  m_edit->show();

  m_commandResponseMessageDisplay->hide();
  m_commandResponseMessageDisplayHide->stop();

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
    // We send to KateViewInternal as it updates the status bar and removes the "?".
    const Qt::Key syntheticSearchCompletedKey = (m_wasAborted ? static_cast<Qt::Key>(0) : Qt::Key_Enter);
    QKeyEvent syntheticSearchCompletedKeyPress(QEvent::KeyPress, syntheticSearchCompletedKey, Qt::NoModifier);
    m_isSendingSyntheticSearchCompletedKeypress = true;
    QApplication::sendEvent(m_view->focusProxy(), &syntheticSearchCompletedKeyPress);
    m_isSendingSyntheticSearchCompletedKeypress = false;
    if (!m_wasAborted)
    {
      m_view->getViInputModeManager()->setLastSearchPattern(m_currentSearchPattern);
      m_view->getViInputModeManager()->setLastSearchCaseSensitive(m_currentSearchIsCaseSensitive);
      m_view->getViInputModeManager()->setLastSearchBackwards(m_currentSearchIsBackwards);
      m_view->getViInputModeManager()->setLastSearchPlacesCursorAtEndOfMatch(m_currentSearchPlacesCursorAtEndOfMatch);
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
    // Re-route this keypress through Vim's central keypress handling area, so that we can use the keypress in e.g.
    // mappings and macros.
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
  const QString textWithoutRangeExpression = withoutRangeExpression();
  const int cursorPositionWithoutRangeExpression = m_edit->cursorPosition() - rangeExpression().length();
  int commandBeforeCursorBegin = cursorPositionWithoutRangeExpression - 1;
  while (commandBeforeCursorBegin >= 0 && (textWithoutRangeExpression[commandBeforeCursorBegin].isLetterOrNumber() || textWithoutRangeExpression[commandBeforeCursorBegin] == '_' || textWithoutRangeExpression[commandBeforeCursorBegin] == '-'))
  {
    commandBeforeCursorBegin--;
  }
  commandBeforeCursorBegin++;
  return textWithoutRangeExpression.mid(commandBeforeCursorBegin, cursorPositionWithoutRangeExpression - commandBeforeCursorBegin);

}

void KateViEmulatedCommandBar::replaceWordBeforeCursorWith(const QString& newWord)
{
  const int wordBeforeCursorStart = m_edit->cursorPosition() - wordBeforeCursor().length();
  const QString newText = m_edit->text().left(m_edit->cursorPosition() - wordBeforeCursor().length()) +
  newWord +
  m_edit->text().mid(m_edit->cursorPosition());
  m_edit->setText(newText);
  m_edit->setCursorPosition(wordBeforeCursorStart + newWord.length());
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

void KateViEmulatedCommandBar::activateSedFindHistoryCompletion()
{
  if (!KateGlobal::self()->viInputModeGlobal()->searchHistory().isEmpty())
  {
    m_currentCompletionType = SedFindHistory;
    m_completionModel->setStringList(reversed(KateGlobal::self()->viInputModeGlobal()->searchHistory()));
    m_completer->setCompletionPrefix(sedFindTerm());
    m_completer->complete();
  }
}

void KateViEmulatedCommandBar::activateSedReplaceHistoryCompletion()
{
  if (!KateGlobal::self()->viInputModeGlobal()->replaceHistory().isEmpty())
  {
    m_currentCompletionType = SedReplaceHistory;
    m_completionModel->setStringList(reversed(KateGlobal::self()->viInputModeGlobal()->replaceHistory()));
    m_completer->setCompletionPrefix(sedReplaceTerm());
    m_completer->complete();
  }
}

void KateViEmulatedCommandBar::deactivateCompletion()
{
  kDebug(13070) << "Manually dismissing completions";
  m_completer->popup()->hide();
  m_currentCompletionType = None;
}

void KateViEmulatedCommandBar::abortCompletionAndResetToPreCompletion()
{
  deactivateCompletion();
  m_isNextTextChangeDueToCompletionChange = true;
  m_edit->setText(m_textToRevertToIfCompletionAborted);
  m_edit->setCursorPosition(m_cursorPosToRevertToIfCompletionAborted);
  m_isNextTextChangeDueToCompletionChange = false;
}

void KateViEmulatedCommandBar::updateCompletionPrefix()
{
  // TODO - switch on type is not very OO - consider making a polymorphic "completion" class.
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
  // Seem to need a call to complete() else the size of the popup box is not altered appropriately.
  m_completer->complete();
}

void KateViEmulatedCommandBar::currentCompletionChanged()
{
  // TODO - switch on type is not very OO - consider making a polymorphic "completion" class.
  const QString newCompletion = m_completer->currentCompletion();
  if (newCompletion.isEmpty())
  {
    return;
  }
  m_isNextTextChangeDueToCompletionChange = true;
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
  else if (m_currentCompletionType == SedFindHistory)
  {
    m_edit->setText(withSedFindTermReplacedWith(withCaseSensitivityMarkersStripped(withSedDelimiterEscaped(newCompletion))));
    ParsedSedExpression parsedSedExpression = parseAsSedExpression();
    m_edit->setCursorPosition(parsedSedExpression.findEndPos + 1);
  }
  else if (m_currentCompletionType == SedReplaceHistory)
  {
    m_edit->setText(withSedReplaceTermReplacedWith(withSedDelimiterEscaped(newCompletion)));
    ParsedSedExpression parsedSedExpression = parseAsSedExpression();
    m_edit->setCursorPosition(parsedSedExpression.replaceEndPos + 1);
  }
  else
  {
    Q_ASSERT(false && "Something went wrong, here - completion with unrecognised completion type");
  }
  m_isNextTextChangeDueToCompletionChange = false;
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

KateViEmulatedCommandBar::ParsedSedExpression KateViEmulatedCommandBar::parseAsSedExpression()
{
  const QString commandWithoutRangeExpression = withoutRangeExpression();
  ParsedSedExpression parsedSedExpression;
  QString delimiter;
  parsedSedExpression.parsedSuccessfully = KateCommands::SedReplace::parse(commandWithoutRangeExpression, delimiter, parsedSedExpression.findBeginPos, parsedSedExpression.findEndPos, parsedSedExpression.replaceBeginPos, parsedSedExpression.replaceEndPos);
  if (parsedSedExpression.parsedSuccessfully)
  {
    parsedSedExpression.delimiter = delimiter.at(0);
    if (parsedSedExpression.replaceBeginPos == -1)
    {
      if (parsedSedExpression.findBeginPos != -1)
      {
        // The replace term was empty, and a quirk of the regex used is that replaceBeginPos will be -1.
        // It's actually the position after the first occurrence of the delimiter after the end of the find pos.
        parsedSedExpression.replaceBeginPos = commandWithoutRangeExpression.indexOf(delimiter, parsedSedExpression.findEndPos) + 1;
        parsedSedExpression.replaceEndPos = parsedSedExpression.replaceBeginPos - 1;
      }
      else
      {
        // Both find and replace terms are empty; replace term is at the third occurrence of the delimiter.
        parsedSedExpression.replaceBeginPos = 0;
        for (int delimiterCount = 1; delimiterCount <= 3; delimiterCount++)
        {
          parsedSedExpression.replaceBeginPos = commandWithoutRangeExpression.indexOf(delimiter, parsedSedExpression.replaceBeginPos + 1);
        }
        parsedSedExpression.replaceEndPos = parsedSedExpression.replaceBeginPos - 1;
      }
    }
    if (parsedSedExpression.findBeginPos == -1)
    {
      // The find term was empty, and a quirk of the regex used is that findBeginPos will be -1.
      // It's actually the position after the first occurrence of the delimiter.
      parsedSedExpression.findBeginPos = commandWithoutRangeExpression.indexOf(delimiter) + 1;
      parsedSedExpression.findEndPos = parsedSedExpression.findBeginPos - 1;
    }

  }

  if (parsedSedExpression.parsedSuccessfully)
  {
    parsedSedExpression.findBeginPos += rangeExpression().length();
    parsedSedExpression.findEndPos += rangeExpression().length();
    parsedSedExpression.replaceBeginPos += rangeExpression().length();
    parsedSedExpression.replaceEndPos += rangeExpression().length();
  }
  return parsedSedExpression;
}

QString KateViEmulatedCommandBar::withSedFindTermReplacedWith(const QString& newFindTerm)
{
  const QString command = m_edit->text();
  ParsedSedExpression parsedSedExpression = parseAsSedExpression();
  Q_ASSERT(parsedSedExpression.parsedSuccessfully);
  return command.mid(0, parsedSedExpression.findBeginPos) +
    newFindTerm +
    command.mid(parsedSedExpression.findEndPos + 1);

}

QString KateViEmulatedCommandBar::withSedReplaceTermReplacedWith(const QString& newReplaceTerm)
{
  const QString command = m_edit->text();
  ParsedSedExpression parsedSedExpression = parseAsSedExpression();
  Q_ASSERT(parsedSedExpression.parsedSuccessfully);
  return command.mid(0, parsedSedExpression.replaceBeginPos) +
    newReplaceTerm +
    command.mid(parsedSedExpression.replaceEndPos + 1);
}

QString KateViEmulatedCommandBar::sedFindTerm()
{
  const QString command = m_edit->text();
  ParsedSedExpression parsedSedExpression = parseAsSedExpression();
  Q_ASSERT(parsedSedExpression.parsedSuccessfully);
  return command.mid(parsedSedExpression.findBeginPos, parsedSedExpression.findEndPos - parsedSedExpression.findBeginPos + 1);
}

QString KateViEmulatedCommandBar::sedReplaceTerm()
{
  const QString command = m_edit->text();
  ParsedSedExpression parsedSedExpression = parseAsSedExpression();
  Q_ASSERT(parsedSedExpression.parsedSuccessfully);
  return command.mid(parsedSedExpression.replaceBeginPos, parsedSedExpression.replaceEndPos - parsedSedExpression.replaceBeginPos + 1);
}

QString KateViEmulatedCommandBar::withSedDelimiterEscaped(const QString& text)
{
  ParsedSedExpression parsedSedExpression = parseAsSedExpression();
  QString delimiterEscaped = ensuredCharEscaped(text, parsedSedExpression.delimiter);
  return delimiterEscaped;
}

bool KateViEmulatedCommandBar::isCursorInFindTermOfSed()
{
  ParsedSedExpression parsedSedExpression = parseAsSedExpression();
  return parsedSedExpression.parsedSuccessfully && (m_edit->cursorPosition() >= parsedSedExpression.findBeginPos && m_edit->cursorPosition() <= parsedSedExpression.findEndPos + 1);
}

bool KateViEmulatedCommandBar::isCursorInReplaceTermOfSed()
{
  ParsedSedExpression parsedSedExpression = parseAsSedExpression();
  return parsedSedExpression.parsedSuccessfully && m_edit->cursorPosition() >= parsedSedExpression.replaceBeginPos && m_edit->cursorPosition() <= parsedSedExpression.replaceEndPos + 1;
}

QString KateViEmulatedCommandBar::withoutRangeExpression()
{
  const QString originalCommand = m_edit->text();
  return originalCommand.mid(rangeExpression().length());
}

QString KateViEmulatedCommandBar::rangeExpression()
{
  QString rangeExpression;
  QString unused;
  const QString command = m_edit->text();
  CommandRangeExpressionParser::parseRangeExpression(command, m_view, rangeExpression, unused);
  return rangeExpression;
}

bool KateViEmulatedCommandBar::handleKeyPress(const QKeyEvent* keyEvent)
{
  if (keyEvent->modifiers() == Qt::ControlModifier && (keyEvent->key() == Qt::Key_C || keyEvent->key() == Qt::Key_BracketLeft) && !m_waitingForRegister)
  {
    if (m_currentCompletionType == None || !m_completer->popup()->isVisible())
    {
      emit hideMe();
    }
    else
    {
      abortCompletionAndResetToPreCompletion();
    }
    return true;
  }
  if (m_interactiveSedReplaceActive)
  {
    // TODO - it would be better to use e.g. keyEvent->key() == Qt::Key_Y instead of keyEvent->text() == "y",
    // but this would require some slightly dicey changes to the "feed key press" code in order to make it work
    // with mappings and macros.
    if (keyEvent->text() == "y" || keyEvent->text() == "n")
    {
      const Cursor cursorPosIfFinalMatch = m_interactiveSedReplacer->currentMatch().start();
      if (keyEvent->text() == "y")
      {
        m_interactiveSedReplacer->replaceCurrentMatch();
      }
      else
      {
        m_interactiveSedReplacer->skipCurrentMatch();
      }
      updateMatchHighlight(m_interactiveSedReplacer->currentMatch());
      updateInteractiveSedReplaceLabelText();
      moveCursorTo(m_interactiveSedReplacer->currentMatch().start());

      if (!m_interactiveSedReplacer->currentMatch().isValid())
      {
        moveCursorTo(cursorPosIfFinalMatch);
        finishInteractiveSedReplace();
      }
      return true;
    }
    else if (keyEvent->text() == "l")
    {
      m_interactiveSedReplacer->replaceCurrentMatch();
      finishInteractiveSedReplace();
      return true;
    }
    else if (keyEvent->text() == "q")
    {
      finishInteractiveSedReplace();
      return true;
    }
    else if (keyEvent->text() == "a")
    {
      m_interactiveSedReplacer->replaceAllRemaining();
      finishInteractiveSedReplace();
      return true;
    }
    return false;
  }
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
        if (isCursorInFindTermOfSed())
        {
          activateSedFindHistoryCompletion();
        }
        else if (isCursorInReplaceTermOfSed())
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
      const QChar key = KateViKeyParser::self()->KeyEventToQChar(*keyEvent).toLower();

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
      if (m_insertedTextShouldBeEscapedForSearchingAsLiteral)
      {
        textToInsert = escapedForSearchingAsLiteral(textToInsert);
        m_insertedTextShouldBeEscapedForSearchingAsLiteral = false;
      }
      m_edit->setText(m_edit->text().insert(m_edit->cursorPosition(), textToInsert));
      m_edit->setCursorPosition(oldCursorPosition + textToInsert.length());
      m_waitingForRegister = false;
      m_waitingForRegisterIndicator->setVisible(false);
    }
  }
  else if ((keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_H) || keyEvent->key() == Qt::Key_Backspace)
  {
    if (m_edit->text().isEmpty())
    {
      emit hideMe();
    }
    m_edit->backspace();
    return true;
  }
  else if (keyEvent->modifiers() == Qt::ControlModifier)
  {
    if (keyEvent->key() == Qt::Key_B)
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
    else if (keyEvent->key() == Qt::Key_R || keyEvent->key() == Qt::Key_G)
    {
      m_waitingForRegister = true;
      m_waitingForRegisterIndicator->setVisible(true);
      if (keyEvent->key() == Qt::Key_G)
      {
        m_insertedTextShouldBeEscapedForSearchingAsLiteral = true;
      }
      return true;
    }
    else if (keyEvent->key() == Qt::Key_D || keyEvent->key() == Qt::Key_F)
    {
      if (m_mode == Command)
      {
        ParsedSedExpression parsedSedExpression = parseAsSedExpression();
        if (parsedSedExpression.parsedSuccessfully)
        {
          const bool clearFindTerm = (keyEvent->key() == Qt::Key_D);
          if (clearFindTerm)
          {
            m_edit->setSelection(parsedSedExpression.findBeginPos, parsedSedExpression.findEndPos - parsedSedExpression.findBeginPos + 1);
            m_edit->insert("");
          }
          else
          {
            // Clear replace term.
            m_edit->setSelection(parsedSedExpression.replaceBeginPos, parsedSedExpression.replaceEndPos - parsedSedExpression.replaceBeginPos + 1);
            m_edit->insert("");
          }
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
        ParsedSedExpression parsedSedExpression = parseAsSedExpression();
        qDebug() << "text:\n" << m_edit->text() << "\n is sed replace: " << parsedSedExpression.parsedSuccessfully;
        if (parsedSedExpression.parsedSuccessfully)
        {
          const QString originalFindTerm = sedFindTerm();
          const QString convertedFindTerm = vimRegexToQtRegexPattern(originalFindTerm);
          const QString commandWithSedSearchRegexConverted = withSedFindTermReplacedWith(convertedFindTerm);
          KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem(originalFindTerm);
          const QString replaceTerm = sedReplaceTerm();
          KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem(replaceTerm);
          commandToExecute = commandWithSedSearchRegexConverted;
          kDebug(13070) << "Command to execute after replacing search term: "<< commandToExecute;
        }

        const QString commandResponseMessage = executeCommand(commandToExecute);
        if (!m_interactiveSedReplaceActive)
        {
          if (commandResponseMessage.isEmpty() && !m_interactiveSedReplaceActive)
          {
            emit hideMe();
          }
          else
          {
            switchToCommandResponseDisplay(commandResponseMessage);
          }
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
    // Send the keypress back to the QLineEdit.  Ideally, instead of doing this, we would simply return "false"
    // and let Qt re-dispatch the event itself; however, there is a corner case in that if the selection
    // changes (as a result of e.g. incremental searches during Visual Mode), and the keypress that causes it
    // is not dispatched from within KateViInputModeHandler::handleKeypress(...)
    // (so KateViInputModeManager::isHandlingKeypress() returns false), we lose information about whether we are
    // in Visual Mode, Visual Line Mode, etc.  See KateViVisualMode::updateSelection( ).
    QKeyEvent keyEventCopy(keyEvent->type(), keyEvent->key(), keyEvent->modifiers(), keyEvent->text(), keyEvent->isAutoRepeat(), keyEvent->count());
    if (!m_interactiveSedReplaceActive)
    {
      qApp->notify(m_edit, &keyEventCopy);
    }
    m_suspendEditEventFiltering = false;
  }
  return true;
}

bool KateViEmulatedCommandBar::isSendingSyntheticSearchCompletedKeypress()
{
  return m_isSendingSyntheticSearchCompletedKeypress;
}

void KateViEmulatedCommandBar::startInteractiveSearchAndReplace(QSharedPointer< KateCommands::SedReplace::InteractiveSedReplacer > interactiveSedReplace)
{
  m_interactiveSedReplaceActive = true;
  m_interactiveSedReplacer = interactiveSedReplace;
  if (!interactiveSedReplace->currentMatch().isValid())
  {
    // Bit of a hack, but we leave m_incrementalSearchAndReplaceActive true, here, else
    // the bar would be immediately hidden and the "0 replacements made" message not shown.
    finishInteractiveSedReplace();
    return;
  }
  kDebug(13070) << "Starting incremental search and replace";
  m_commandResponseMessageDisplay->hide();
  m_edit->hide();
  m_barTypeIndicator->hide();
  m_interactiveSedReplaceLabel->show();
  updateMatchHighlight(interactiveSedReplace->currentMatch());
  updateInteractiveSedReplaceLabelText();
  moveCursorTo(interactiveSedReplace->currentMatch().start());
}

void KateViEmulatedCommandBar::showBarTypeIndicator(KateViEmulatedCommandBar::Mode mode)
{
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
  m_barTypeIndicator->show();
}

QString KateViEmulatedCommandBar::executeCommand(const QString& commandToExecute)
{
  // TODO - this is a hack-ish way of finding the response from the command; maybe
  // add another overload of "execute" to KateCommandLineBar that returns the
  // response message ... ?
  m_view->cmdLineBar()->setText("");
  m_view->cmdLineBar()->execute(commandToExecute);
  KateCmdLineEdit *kateCommandLineEdit = m_view->cmdLineBar()->findChild<KateCmdLineEdit*>();
  Q_ASSERT(kateCommandLineEdit);
  const QString commandResponseMessage = kateCommandLineEdit->text();
  return commandResponseMessage;
}

void KateViEmulatedCommandBar::switchToCommandResponseDisplay(const QString& commandResponseMessage)
{
  // Display the message for a while.  Become inactive, so we don't steal keys in the meantime.
  m_isActive = false;
  m_edit->hide();
  m_interactiveSedReplaceLabel->hide();
  m_barTypeIndicator->hide();
  m_commandResponseMessageDisplay->show();
  m_commandResponseMessageDisplay->setText(commandResponseMessage);
  m_commandResponseMessageDisplayHide->start(m_commandResponseMessageTimeOutMS);
}

void KateViEmulatedCommandBar::updateInteractiveSedReplaceLabelText()
{
  m_interactiveSedReplaceLabel->setText(m_interactiveSedReplacer->currentMatchReplacementConfirmationMessage() + " (y/n/a/q/l)");
}

void KateViEmulatedCommandBar::finishInteractiveSedReplace()
{
  switchToCommandResponseDisplay(m_interactiveSedReplacer->finalStatusReportMessage());
  m_interactiveSedReplacer.clear();
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
  Q_ASSERT(!m_interactiveSedReplaceActive);
  qDebug() << "New text: " << newText;
  if (!m_isNextTextChangeDueToCompletionChange)
  {
    m_textToRevertToIfCompletionAborted = newText;
    m_cursorPosToRevertToIfCompletionAborted = m_edit->cursorPosition();
  }
  if (m_mode == SearchForward || m_mode == SearchBackward)
  {
    QString qtRegexPattern = newText;
    const bool searchBackwards = (m_mode == SearchBackward);
    const bool placeCursorAtEndOfMatch = shouldPlaceCursorAtEndOfMatch(qtRegexPattern, searchBackwards);
    if (isRepeatLastSearch(qtRegexPattern, searchBackwards))
    {
      qtRegexPattern = m_view->getViInputModeManager()->getLastSearchPattern();
    }
    else
    {
      qtRegexPattern = withSearchConfigRemoved(qtRegexPattern, searchBackwards);
      qtRegexPattern = vimRegexToQtRegexPattern(qtRegexPattern);
    }

    // Decide case-sensitivity via SmartCase (note: if the expression contains \C, the "case-sensitive" marker, then
    // we will be case-sensitive "by coincidence", as it were.).
    bool caseSensitive = true;
    if (qtRegexPattern.toLower() == qtRegexPattern)
    {
      caseSensitive = false;
    }

    qtRegexPattern = withCaseSensitivityMarkersStripped(qtRegexPattern);

    qDebug() << "Final regex: " << qtRegexPattern;

    m_currentSearchPattern = qtRegexPattern;
    m_currentSearchIsCaseSensitive = caseSensitive;
    m_currentSearchIsBackwards = searchBackwards;
    m_currentSearchPlacesCursorAtEndOfMatch = placeCursorAtEndOfMatch;

    // The "count" for the current search is not shared between Visual & Normal mode, so we need to pick
    // the right one to handle the counted search.
    Range match = m_view->getViInputModeManager()->getCurrentViModeHandler()->findPattern(qtRegexPattern, searchBackwards, caseSensitive, m_startingCursorPos);

    if (match.isValid())
    {
      // The returned range ends one past the last character of the match, so adjust.
      Cursor realMatchEnd = Cursor(match.end().line(), match.end().column() - 1);
      if (realMatchEnd.column() == -1)
      {
        realMatchEnd = Cursor(realMatchEnd.line() - 1, m_view->doc()->lineLength(realMatchEnd.line() - 1));
      }
      moveCursorTo(placeCursorAtEndOfMatch ? realMatchEnd :  match.start());
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
  if (m_mode == Command && m_currentCompletionType == None && !withoutRangeExpression().isEmpty())
  {
    activateCommandCompletion();
  }

  // Command completion mode should be automatically invoked if we are in Command mode, but
  // only if this is the leading word in the text edit (it gets annoying if completion pops up
  // after ":s/se" etc).
  const bool commandBeforeCursorIsLeading = (m_edit->cursorPosition() - commandBeforeCursor().length() == rangeExpression().length());
  if (m_mode == Command && !commandBeforeCursorIsLeading && m_currentCompletionType == Commands && !m_isNextTextChangeDueToCompletionChange)
  {
    deactivateCompletion();
  }

  // If we edit the text after having selected a completion, this means we implicitly accept it,
  // and so we should dismiss it.
  if (!m_isNextTextChangeDueToCompletionChange && m_completer->popup()->currentIndex().row() != -1)
  {
    deactivateCompletion();
  }

  if (m_currentCompletionType != None && !m_isNextTextChangeDueToCompletionChange)
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

