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
}

KateViEmulatedCommandBar::KateViEmulatedCommandBar(KateView* view, QWidget* parent)
    : KateViewBarWidget(false, parent),
      m_isActive(false),
      m_view(view),
      m_doNotResetCursorOnClose(false),
      m_suspendEditEventFiltering(false),
      m_waitingForRegister(false),
      m_currentCompletionType(None)
{
  QVBoxLayout * layout = new QVBoxLayout();
  centralWidget()->setLayout(layout);
  m_barTypeIndicator = new QLabel(this);
  m_barTypeIndicator->setObjectName("bartypeindicator");

  m_edit = new QLineEdit(this);
  m_edit->setObjectName("commandtext");
  layout->addWidget(m_edit);

  m_searchBackwards = false;

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
  m_searchHistoryModel = new QStringListModel;
  m_completer->setModel(m_searchHistoryModel);
  m_completer->setCaseSensitivity(Qt::CaseInsensitive);
  m_completer->popup()->installEventFilter(this);
}

KateViEmulatedCommandBar::~KateViEmulatedCommandBar()
{
  delete m_highlightedMatch;
}

void KateViEmulatedCommandBar::init(bool backwards)
{
  m_currentCompletionType = None;
  if (backwards)
  {
    m_barTypeIndicator->setText("?");
    m_searchBackwards = true;
  }
  else
  {
    m_barTypeIndicator->setText("/");
    m_searchBackwards = false;
  }
  m_edit->setFocus();
  m_edit->clear();
  m_startingCursorPos = m_view->cursorPosition();
  m_isActive = true;
}

bool KateViEmulatedCommandBar::isActive()
{
  return m_isActive;
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

  // Send a synthetic keypress through the system that signals whether the search was aborted or
  // not.  If not, the keypress will "complete" the search motion, thus triggering it.
  const Qt::Key syntheticSearchCompletedKey = (wasDismissed ? Qt::Key_Escape : Qt::Key_Enter);
  QKeyEvent syntheticSearchCompletedKeyPress(QEvent::KeyPress, syntheticSearchCompletedKey, Qt::NoModifier);
  m_view->getViInputModeManager()->handleKeypress(&syntheticSearchCompletedKeyPress);
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
  while (wordBeforeCursorBegin >= 0 && (m_edit->text()[wordBeforeCursorBegin].isLetterOrNumber() || m_edit->text()[wordBeforeCursorBegin] == '_' ))
  {
    wordBeforeCursorBegin--;
  }
  wordBeforeCursorBegin++;
  return m_edit->text().mid(wordBeforeCursorBegin, m_edit->cursorPosition() - wordBeforeCursorBegin);
}

void KateViEmulatedCommandBar::replaceWordBeforeCursorWith(const QString& newWord)
{
  const QString newText = m_edit->text().left(m_edit->cursorPosition() - wordBeforeCursor().length()) +
  newWord +
  m_edit->text().mid(m_edit->cursorPosition());
  m_edit->setText(newText);
}

QString KateViEmulatedCommandBar::vimRegexToQtRegexPattern(const QString& vimRegexPattern)
{
  QString qtRegexPattern = vimRegexPattern;
  qtRegexPattern = toggledEscaped(qtRegexPattern, '(');
  qtRegexPattern = toggledEscaped(qtRegexPattern, ')');
  qtRegexPattern = toggledEscaped(qtRegexPattern, '+');
  qtRegexPattern = toggledEscaped(qtRegexPattern, '|');

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
    int previousNonMatchingSquareBracketPos = 0;
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

QString KateViEmulatedCommandBar::toggledEscaped(const QString& originalString, QChar escapeChar)
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

QString KateViEmulatedCommandBar::ensuredCharEscaped(const QString& originalString, QChar charToEscape)
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

void KateViEmulatedCommandBar::populateAndShowSearchHistoryCompletion()
{
  m_currentCompletionType = SearchHistory;
  QStringList searchHistoryReversed;
  foreach(QString searchHistoryItem, KateGlobal::self()->viInputModeGlobal()->searchHistory())
  {
    searchHistoryReversed.prepend(searchHistoryItem);
  }
  m_searchHistoryModel->setStringList(searchHistoryReversed);
  updateCompletionPrefix();
  m_completer->complete();
}

void KateViEmulatedCommandBar::populateAndShowWordFromDocumentCompletion()
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
    m_searchHistoryModel->setStringList(foundWords);
    updateCompletionPrefix();
    m_completer->complete();
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
  // Seem to need this to alter the size of the popup box appropriately.
  m_completer->complete();
}

void KateViEmulatedCommandBar::completionChosen()
{
  if (m_currentCompletionType == WordFromDocument)
  {
    replaceWordBeforeCursorWith(m_completer->currentCompletion());
  }
  else if (m_currentCompletionType == SearchHistory)
  {
    m_edit->setText(m_completer->currentCompletion());
  }
  else
  {
    Q_ASSERT("Something went wrong, here - completion with unrecognised completion type");
  }
  m_completer->popup()->hide();
  m_currentCompletionType = None;
}

void KateViEmulatedCommandBar::setCompletionIndex(int index)
{
  QModelIndex modelIndex = m_completer->popup()->model()->index(index, 0);
  // Need to set both of these, for some reason.
  m_completer->popup()->setCurrentIndex(modelIndex);
  m_completer->setCurrentRow(index);

  m_completer->popup()->scrollTo(modelIndex);
}

bool KateViEmulatedCommandBar::handleKeyPress(const QKeyEvent* keyEvent)
{
  if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
  {
    if (m_completer->popup()->isVisible())
    {
      completionChosen();
      return true;
    }
  }
  if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_Space)
  {
    populateAndShowWordFromDocumentCompletion();
  }
  if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_P)
  {
    if (!m_completer->popup()->isVisible())
    {
      populateAndShowSearchHistoryCompletion();
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
      populateAndShowSearchHistoryCompletion();
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
  } else if (keyEvent->modifiers() == Qt::ControlModifier)
  {
    if (keyEvent->key() == Qt::Key_C || keyEvent->key() == Qt::Key_BracketLeft)
    {
      if (m_currentCompletionType == None)
      {
        emit hideMe();
      }
      else
      {
        m_completer->popup()->hide();
        m_currentCompletionType = None;
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
    m_doNotResetCursorOnClose =  true;
    emit hideMe();
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
  const QString qtRegexPattern = vimRegexToQtRegexPattern(newText);

  qDebug() << "Final regex: " << qtRegexPattern;

  // Decide case-sensitivity via SmartCase.
  bool caseSensitive = true;
  if (qtRegexPattern.toLower() == qtRegexPattern)
  {
    caseSensitive = false;
  }

  m_view->getViInputModeManager()->setLastSearchPattern(qtRegexPattern);
  m_view->getViInputModeManager()->setLastSearchCaseSensitive(caseSensitive);
  m_view->getViInputModeManager()->setLastSearchBackwards(m_searchBackwards);

  KateViModeBase* currentModeHandler = (m_view->getCurrentViMode() == NormalMode) ? static_cast<KateViModeBase*>(m_view->getViInputModeManager()->getViNormalMode()) : static_cast<KateViModeBase*>(m_view->getViInputModeManager()->getViVisualMode());
  Range match = currentModeHandler->findPattern(qtRegexPattern, m_searchBackwards, caseSensitive, m_startingCursorPos);

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

  if (m_currentCompletionType != None)
  {
    updateCompletionPrefix();
  }
}
