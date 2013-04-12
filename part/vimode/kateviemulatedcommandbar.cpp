#include "kateviemulatedcommandbar.h"
#include "katevikeyparser.h"
#include "kateview.h"
#include "kateviglobal.h"
#include "kateglobal.h"
#include "kateconfig.h"

#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QApplication>

KateViEmulatedCommandBar::KateViEmulatedCommandBar(KateView* view, QWidget* parent)
    : KateViewBarWidget(false, parent),
      m_view(view),
      m_doNotResetCursorOnClose(false),
      m_suspendEditEventFiltering(false),
      m_waitingForRegister(false)
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
}

KateViEmulatedCommandBar::~KateViEmulatedCommandBar()
{
  delete m_highlightedMatch;
}

void KateViEmulatedCommandBar::init(bool backwards)
{
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
  }
  m_startingCursorPos = KTextEditor::Cursor::invalid();
  m_doNotResetCursorOnClose = false;
  updateMatchHighlight(Range::invalid());
}

void KateViEmulatedCommandBar::updateMatchHighlightAttrib()
{
  const QColor& matchColour = m_view->renderer()->config()->searchHighlightColor();
  if (!m_highlightMatchAttribute)
  {
    m_highlightMatchAttribute = new KTextEditor::Attribute;
  }
  qDebug() << "matchColour:" << matchColour;
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

void KateViEmulatedCommandBar::deleteNonSpacesToLeftOfCursor()
{
  while (m_edit->cursorPosition() != 0 && m_edit->text()[m_edit->cursorPosition() - 1] != ' ')
  {
    m_edit->backspace();
  }
}

QString KateViEmulatedCommandBar::vimRegexToQtRegexPattern(const QString& vimRegexPattern)
{
  QString qtRegexPattern = vimRegexPattern;
  qtRegexPattern = toggledEscaped(qtRegexPattern, '(');
  qtRegexPattern = toggledEscaped(qtRegexPattern, ')');
  qtRegexPattern = toggledEscaped(qtRegexPattern, '+');
  qtRegexPattern = toggledEscaped(qtRegexPattern, '|');

  bool lookingForMatchingCloseBracket = false;
  bool treatSquareBracketsAsLiterals = true;
  for (int i = 0; i < qtRegexPattern.length(); i++)
  {
    if (qtRegexPattern[i] == '[')
    {
      lookingForMatchingCloseBracket = true;
    }
    if (qtRegexPattern[i] == ']' && lookingForMatchingCloseBracket)
    {
      treatSquareBracketsAsLiterals = false;
    }
  }
  if(treatSquareBracketsAsLiterals)
  {
    qtRegexPattern = ensuredCharEscaped(qtRegexPattern, '[');
    qtRegexPattern = ensuredCharEscaped(qtRegexPattern, ']');
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


bool KateViEmulatedCommandBar::handleKeyPress(const QKeyEvent* keyEvent)
{
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
      emit hideMe();
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
      deleteNonSpacesToLeftOfCursor();
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
  m_view->getViInputModeManager()->setLastSearchPattern(qtRegexPattern);

  KTextEditor::Search::SearchOptions searchOptions;
  if (qtRegexPattern.toLower() == qtRegexPattern)
  {
    searchOptions |= KTextEditor::Search::CaseInsensitive;
    m_view->getViInputModeManager()->setLastSearchCaseSensitive(false);
  }
  else
  {
    m_view->getViInputModeManager()->setLastSearchCaseSensitive(true);
  }
  searchOptions |= KTextEditor::Search::Regex;

  // TODO - merge some of this with KateViNormalMode::findPattern(...)
  if (!m_searchBackwards)
  {
    m_view->getViInputModeManager()->setLastSearchBackwards(false);
    const KTextEditor::Range matchRange = m_view->doc()->searchText(KTextEditor::Range(Cursor(m_startingCursorPos.line(), m_startingCursorPos.column() + 1), m_view->doc()->documentEnd()), qtRegexPattern, searchOptions).first();

    updateMatchHighlight(matchRange);

    if (matchRange.isValid())
    {
      m_view->setCursorPosition(matchRange.start());
    }
    else
    {
      // Wrap around.
      const KTextEditor::Range wrappedMatchRange = m_view->doc()->searchText(KTextEditor::Range(m_view->doc()->documentRange().start(), m_view->doc()->documentEnd()), qtRegexPattern, searchOptions).first();
      if (wrappedMatchRange.isValid())
      {
        m_view->setCursorPosition(wrappedMatchRange.start());
        updateMatchHighlight(wrappedMatchRange);
      }
      else
      {
        m_view->setCursorPosition(m_startingCursorPos);
      }
    }
  }
  else
  {
    m_view->getViInputModeManager()->setLastSearchBackwards(true);
    searchOptions |= KTextEditor::Search::Backwards;
    // Ok - this is trickier: we can't search in the range from doc start to m_startingCursorPos, because
    // the match might extend *beyond* m_startingCursorPos.
    // We could search through the entire document and then filter out only those matches that are
    // after m_startingCursorPos, but it's more efficient to instead search from the start of the
    // document until the beginning of the line after m_startingCursorPos, and then filter.
    // Unfortunately, searchText doesn't necessarily turn up all matches (just the first one, sometimes)
    // so we must repeatedly search in such a way that the previous match isn't found, until we either
    // find no matches at all, or the first match that is before m_startingCursorPos.
    Cursor searchBegin = Cursor(m_startingCursorPos.line(), m_view->doc()->lineLength(m_startingCursorPos.line()));
    Range bestMatch = Range::invalid();
    while (true)
    {
      QVector<Range> matchesUnfiltered = m_view->doc()->searchText(Range(searchBegin, m_view->doc()->documentRange().start()), qtRegexPattern, searchOptions);

      if (matchesUnfiltered.size() == 1 && !matchesUnfiltered.first().isValid())
      {
        break;
      }

      // After sorting, the last element in matchesUnfiltered is the last match position.
      qSort(matchesUnfiltered);

      QVector<Range> filteredMatches;
      foreach(Range unfilteredMatch, matchesUnfiltered)
      {
        if (unfilteredMatch.start() < m_startingCursorPos)
        {
          filteredMatches.append(unfilteredMatch);
        }
      }
      if (!filteredMatches.isEmpty())
      {
        // Want the latest matching range that is before m_startingCursorPos.
        bestMatch = filteredMatches.last();
        break;
      }

      // We found some unfiltered matches, but none were suitable. In case matchesUnfiltered wasn't
      // all matching elements, search again, starting from before the earliest matching range.
      if (filteredMatches.isEmpty())
      {
        searchBegin = matchesUnfiltered.first().start();
      }
    }

    Range matchRange = bestMatch;

    updateMatchHighlight(matchRange);

    if (matchRange.isValid())
    {
      m_view->setCursorPosition(matchRange.start());
    }
    else
    {
      const KTextEditor::Range wrappedMatchRange = m_view->doc()->searchText(KTextEditor::Range(m_view->doc()->documentEnd(), m_view->doc()->documentRange().start()), qtRegexPattern, searchOptions).first();


      if (wrappedMatchRange.isValid())
      {
        m_view->setCursorPosition(wrappedMatchRange.start());
        updateMatchHighlight(wrappedMatchRange);
      }
      else
      {
        m_view->setCursorPosition(m_startingCursorPos);
      }
    }
  }
}
