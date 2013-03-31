#include "kateviemulatedcommandbar.h"
#include "kateview.h"

#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QApplication>

KateViEmulatedCommandBar::KateViEmulatedCommandBar(KateView* view, QWidget* parent)
    : KateViewBarWidget(false, parent),
      m_view(view),
      m_doNotResetCursorOnClose(false),
      m_suspendEditEventFiltering(false)
{
  QVBoxLayout * layout = new QVBoxLayout();
  centralWidget()->setLayout(layout);
  m_barTypeIndicator = new QLabel(this);
  m_barTypeIndicator->setObjectName("bartypeindicator");

  m_edit = new QLineEdit(this);
  m_edit->setObjectName("commandtext");
  layout->addWidget(m_edit);

  m_searchBackwards = false;

  m_edit->installEventFilter(this);
  connect(m_edit, SIGNAL(textChanged(QString)), this, SLOT(editTextChanged(QString)));
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

bool KateViEmulatedCommandBar::handleKeyPress(const QKeyEvent* keyEvent)
{
  if (keyEvent->modifiers() == Qt::ControlModifier)
  {
    if (keyEvent->key() == Qt::Key_C || keyEvent->key() == Qt::Key_BracketLeft)
    {
      emit hideMe();
      return true;
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
  m_view->getViInputModeManager()->setLastSearchPattern(newText);
  KTextEditor::Search::SearchOptions searchOptions;

  if (newText.toLower() == newText)
  {
    searchOptions |= KTextEditor::Search::CaseInsensitive;
    m_view->getViInputModeManager()->setLastSearchCaseSensitive(false);
  }
  else
  {
    m_view->getViInputModeManager()->setLastSearchCaseSensitive(true);
  }
  searchOptions |= KTextEditor::Search::Regex;

  if (!m_searchBackwards)
  {
    m_view->getViInputModeManager()->setLastSearchBackwards(false);
    const KTextEditor::Cursor matchPos = m_view->doc()->searchText(KTextEditor::Range(m_startingCursorPos, m_view->doc()->documentEnd()), newText, searchOptions).first().start();
    m_view->setCursorPosition(matchPos);

    if (matchPos.isValid())
    {
      m_view->setCursorPosition(matchPos);
    }
    else
    {
      // Wrap around.
      const KTextEditor::Cursor wrappedMatchPos = m_view->doc()->searchText(KTextEditor::Range(m_view->doc()->documentRange().start(), m_view->doc()->documentEnd()), newText, searchOptions).first().start();
      if (wrappedMatchPos.isValid())
      {
        m_view->setCursorPosition(wrappedMatchPos);
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
    const KTextEditor::Cursor matchPos = m_view->doc()->searchText(KTextEditor::Range(m_startingCursorPos, m_view->doc()->documentRange().start()), newText, searchOptions).first().start();
    if (matchPos.isValid())
    {
      m_view->setCursorPosition(matchPos);
    }
    else
    {
      const KTextEditor::Cursor wrappedMatchPos = m_view->doc()->searchText(KTextEditor::Range(m_view->doc()->documentEnd(), m_startingCursorPos), newText, searchOptions).first().start();
      if (wrappedMatchPos.isValid())
      {
        m_view->setCursorPosition(wrappedMatchPos);
      }
      else
      {
        m_view->setCursorPosition(m_startingCursorPos);
      }
    }
  }
}
