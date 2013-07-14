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

#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QCompleter>
#include <QApplication>
#include <KDE/KColorScheme>

namespace
{
  bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
  {
    return s1.toLower() < s2.toLower();
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
      if (indexOfEscapeChar == 0 || toggledEscapedString[indexOfEscapeChar - 1] != '\\')
      {
        // Escape.
        toggledEscapedString.replace(indexOfEscapeChar, 1, QString("\\") + escapeChar);
        searchFrom = indexOfEscapeChar + 2;
      }
      else
      {
        // Unescape.
        toggledEscapedString.remove(indexOfEscapeChar - 1, 1);
        searchFrom = indexOfEscapeChar + 1;
      }
    } while (true);

    return toggledEscapedString;
  }

  QString ensuredCharEscaped(const QString& originalString, QChar charToEscape)
  {
      QString escapedString = originalString;
      for (int i = 0; i < escapedString.length(); i++)
      {
        if (escapedString[i] == charToEscape && (i == 0 || escapedString[i - 1] != '\\'))
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
      if (qtRegexPattern[i] == '[' && !lookingForMatchingCloseBracket)
      {
        lookingForMatchingCloseBracket = true;
        openingBracketPos = i;
      }
      if (qtRegexPattern[i] == ']' && lookingForMatchingCloseBracket && qtRegexPattern[i - 1] != '\\')
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
}

KateViEmulatedCommandBar::KateViEmulatedCommandBar(KateView* view, QWidget* parent)
    : KateViewBarWidget(false, parent),
      m_isActive(false),
      m_mode(NoMode),
      m_view(view),
      m_doNotResetCursorOnClose(false),
      m_suspendEditEventFiltering(false),
      m_waitingForRegister(false),
      m_commandResponseMessageTimeOutMS(4000),
      m_nextTextChangeDueToCompletionChange(false),
      m_currentCompletionType(None)
{
  QVBoxLayout * layout = new QVBoxLayout();
  centralWidget()->setLayout(layout);
  m_barTypeIndicator = new QLabel(this);
  m_barTypeIndicator->setObjectName("bartypeindicator");

  m_edit = new QLineEdit(this);
  m_edit->setObjectName("commandtext");
  layout->addWidget(m_edit);

  m_commandResponseMessageDisplay = new QLabel(this);
  m_commandResponseMessageDisplay->setObjectName("commandresponsemessage");
  layout->addWidget(m_commandResponseMessageDisplay);

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
      Q_ASSERT("Unknown mode!");
  }
  m_barTypeIndicator->setText(barTypeIndicator);

  m_mode = mode;
  m_edit->setFocus();
  m_edit->setText(initialText);
  m_edit->show();

  m_commandResponseMessageDisplay->hide();

  m_startingCursorPos = m_view->cursorPosition();
  m_isActive = true;

  if (mode == Command)
  {
    activateCommandCompletion();
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
    if (!m_doNotResetCursorOnClose)
    {
      m_view->setCursorPosition(m_startingCursorPos);
    }
    KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem(m_edit->text());
  }
  m_startingCursorPos = KTextEditor::Cursor::invalid();
  const bool wasDismissed = !m_doNotResetCursorOnClose;
  m_doNotResetCursorOnClose = false;
  updateMatchHighlight(Range::invalid());
  m_completer->popup()->hide();
  m_isActive = false;

  if (m_mode == SearchForward || m_mode == SearchBackward)
  {
    // Send a synthetic keypress through the system that signals whether the search was aborted or
    // not.  If not, the keypress will "complete" the search motion, thus triggering it.
    const Qt::Key syntheticSearchCompletedKey = (wasDismissed ? Qt::Key_Escape : Qt::Key_Enter);
    QKeyEvent syntheticSearchCompletedKeyPress(QEvent::KeyPress, syntheticSearchCompletedKey, Qt::NoModifier);
    m_view->getViInputModeManager()->handleKeypress(&syntheticSearchCompletedKeyPress);
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

bool KateViEmulatedCommandBar::eventFilter(QObject* object, QEvent* event)
{
  if (m_suspendEditEventFiltering)
  {
    return false;
  }
  Q_UNUSED(object);
  if (event->type() == QEvent::KeyPress)
  {
    m_view->getViInputModeManager()->handleKeypress(static_cast<QKeyEvent*>(event));
    return true;
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
  int commandBeforeCursorBegin = m_edit->cursorPosition() - 1;
  while (commandBeforeCursorBegin >= 0 && (m_edit->text()[commandBeforeCursorBegin].isLetterOrNumber() || m_edit->text()[commandBeforeCursorBegin] == '_' || m_edit->text()[commandBeforeCursorBegin] == '-'))
  {
    commandBeforeCursorBegin--;
  }
  commandBeforeCursorBegin++;
  return m_edit->text().mid(commandBeforeCursorBegin, m_edit->cursorPosition() - commandBeforeCursorBegin);

}

void KateViEmulatedCommandBar::replaceWordBeforeCursorWith(const QString& newWord)
{
  const QString newText = m_edit->text().left(m_edit->cursorPosition() - wordBeforeCursor().length()) +
  newWord +
  m_edit->text().mid(m_edit->cursorPosition());
  m_edit->setText(newText);
}

void KateViEmulatedCommandBar::activateSearchHistoryCompletion()
{
  m_currentCompletionType = SearchHistory;
  QStringList searchHistoryReversed;
  foreach(QString searchHistoryItem, KateGlobal::self()->viInputModeGlobal()->searchHistory())
  {
    searchHistoryReversed.prepend(searchHistoryItem);
  }
  m_completionModel->setStringList(searchHistoryReversed);
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

void KateViEmulatedCommandBar::deactivateCompletion()
{
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
  else if (m_currentCompletionType == Commands)
  {
    m_completer->setCompletionPrefix(commandBeforeCursor());
  }
  // Seem to need this to alter the size of the popup box appropriately.
  m_completer->complete();
}

void KateViEmulatedCommandBar::currentCompletionChanged()
{
  m_nextTextChangeDueToCompletionChange = true;
  if (m_currentCompletionType == WordFromDocument)
  {
    replaceWordBeforeCursorWith(m_completer->currentCompletion());
  }
  else if (m_currentCompletionType == SearchHistory)
  {
    m_edit->setText(m_completer->currentCompletion());
  }
  else if (m_currentCompletionType == Commands)
  {
    m_edit->setText(m_completer->currentCompletion());
  }
  else
  {
    Q_ASSERT("Something went wrong, here - completion with unrecognised completion type");
  }
  m_nextTextChangeDueToCompletionChange = false;
}

void KateViEmulatedCommandBar::setCompletionIndex(int index)
{
  QModelIndex modelIndex = m_completer->popup()->model()->index(index, 0);
  // Need to set both of these, for some reason.
  m_completer->popup()->setCurrentIndex(modelIndex);
  m_completer->setCurrentRow(index);

  m_completer->popup()->scrollTo(modelIndex);

  currentCompletionChanged();
}

bool KateViEmulatedCommandBar::handleKeyPress(const QKeyEvent* keyEvent)
{
  if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_Space)
  {
    activateWordFromDocumentCompletion();
  }
  if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_P)
  {
    if (!m_completer->popup()->isVisible())
    {
      activateSearchHistoryCompletion();
      setCompletionIndex(0);
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
  }
  if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_N)
  {
    if (!m_completer->popup()->isVisible())
    {
      activateSearchHistoryCompletion();
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
  }
  if (m_waitingForRegister)
  {
    if (keyEvent->key() != Qt::Key_Shift)
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
        const CompletionType abortedCompletionType = m_currentCompletionType;
        deactivateCompletion();
        m_nextTextChangeDueToCompletionChange = true;
        if (abortedCompletionType == SearchHistory || abortedCompletionType == Commands)
        {
          m_edit->setText(m_revertToIfCompletionAborted);
        }
        else
        {
          replaceWordBeforeCursorWith(m_revertToIfCompletionAborted);
        }
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
    else if (keyEvent->key() == Qt::Key_W)
    {
      deleteSpacesToLeftOfCursor();
      if(!deleteNonWordCharsToLeftOfCursor())
      {
        deleteWordCharsToLeftOfCursor();
      }
    }
    else if (keyEvent->key() == Qt::Key_R)
    {
      m_waitingForRegister = true;
    }
  }
  else if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
  {
    if (m_completer->popup()->isVisible() && m_currentCompletionType == KateViEmulatedCommandBar::WordFromDocument)
    {
      deactivateCompletion();
    }
    else
    {
      m_doNotResetCursorOnClose =  true;
      if (m_mode == Command)
      {
        kDebug(13070) << "Executing: " << m_edit->text();
        m_commandResponseMessage.clear();
        // TODO - this is a hack-ish way of finding the response from the command; maybe
        // add another overload of "execute" to KateCommandLineBar that returns the
        // response message ... ?
        m_view->cmdLineBar()->setText("");
        m_view->cmdLineBar()->execute(m_edit->text());
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
          QTimer::singleShot(m_commandResponseMessageTimeOutMS, this, SIGNAL(hideMe()));
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
  return false;
}

void KateViEmulatedCommandBar::editTextChanged(const QString& newText)
{
  qDebug() << "New text: " << newText;
  if (!m_nextTextChangeDueToCompletionChange)
  {
    if (m_currentCompletionType != WordFromDocument)
    {
      m_revertToIfCompletionAborted = newText;
    }
    else
    {
      m_revertToIfCompletionAborted = wordBeforeCursor();
    }
  }
  if (m_mode == SearchForward || m_mode == SearchBackward)
  {
    const QString qtRegexPattern = vimRegexToQtRegexPattern(newText);

    qDebug() << "Final regex: " << qtRegexPattern;

    // Decide case-sensitivity via SmartCase.
    bool caseSensitive = true;
    if (qtRegexPattern.toLower() == qtRegexPattern)
    {
      caseSensitive = false;
    }

    const bool searchBackwards = (m_mode == SearchBackward);

    m_view->getViInputModeManager()->setLastSearchPattern(qtRegexPattern);
    m_view->getViInputModeManager()->setLastSearchCaseSensitive(caseSensitive);
    m_view->getViInputModeManager()->setLastSearchBackwards(searchBackwards);

    KateViModeBase* currentModeHandler = (m_view->getCurrentViMode() == NormalMode) ? static_cast<KateViModeBase*>(m_view->getViInputModeManager()->getViNormalMode()) : static_cast<KateViModeBase*>(m_view->getViInputModeManager()->getViVisualMode());
    Range match = currentModeHandler->findPattern(qtRegexPattern, searchBackwards, caseSensitive, m_startingCursorPos);

    QPalette barBackground(m_edit->palette());
    if (match.isValid())
    {
      m_view->setCursorPosition(match.start());
      KColorScheme::adjustBackground(barBackground, KColorScheme::PositiveBackground);
    }
    else
    {
      m_view->setCursorPosition(m_startingCursorPos);
      if (!m_edit->text().isEmpty())
      {
        KColorScheme::adjustBackground(barBackground, KColorScheme::NegativeBackground);
      }
      else
      {
        // Reset to back to normal.
        barBackground = QPalette();
      }
    }
    m_edit->setPalette(barBackground);

    updateMatchHighlight(match);
  }

  // Command completion doesn't need to be manually invoked.
  if (m_mode == Command && m_currentCompletionType == None)
  {
    activateCommandCompletion();
  }

  // Command completion mode should be automatically invoked if we are in Command mode, but
  // only if this is the leading word in the text edit (it gets annoying if completion pops up
  // after ":s/se" etc).
  const bool commandBeforeCursorIsLeading = (m_edit->cursorPosition() - commandBeforeCursor().length() == 0);
  if (m_mode == Command && !commandBeforeCursorIsLeading)
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