#include "kateviemulatedcommandbar.h"
#include "kateview.h"

#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>

KateViEmulatedCommandBar::KateViEmulatedCommandBar(KateView* view, QWidget* parent)
    : KateViewBarWidget(false, parent),
      m_view(view),
      m_pendingCloseIsDueToEnter(false)
{
  QVBoxLayout * layout = new QVBoxLayout();
  centralWidget()->setLayout(layout);
  QLabel *barTypeIndicator = new QLabel(this);
  barTypeIndicator->setObjectName("bartypeindicator");
  barTypeIndicator->setText("/");

  m_edit = new QLineEdit(this);
  m_edit->setObjectName("commandtext");
  layout->addWidget(m_edit);

  m_edit->installEventFilter(this);
  connect(m_edit, SIGNAL(textChanged(QString)), this, SLOT(editTextChanged(QString)));
}

void KateViEmulatedCommandBar::init()
{
  m_edit->setFocus();
  m_edit->clear();
  m_startingCursorPos = m_view->cursorPosition();
}

void KateViEmulatedCommandBar::closed()
{
  // Close can be called multiple times between init()'s, so only reset the cursor once!
  if (m_startingCursorPos.isValid())
  {
    if (!m_pendingCloseIsDueToEnter)
    {
      m_view->setCursorPosition(m_startingCursorPos);
    }
  }
  m_startingCursorPos = KTextEditor::Cursor::invalid();
  m_pendingCloseIsDueToEnter = false;
}

bool KateViEmulatedCommandBar::eventFilter(QObject* object, QEvent* event)
{
  Q_UNUSED(object);
  if (event->type() == QEvent::KeyPress)
  {
     QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
     if (keyEvent->modifiers() == Qt::ControlModifier)
     {
       if (keyEvent->key() == Qt::Key_C || keyEvent->key() == Qt::Key_BracketLeft)
       {
         emit hideMe();
         return true;
       }
     }
     else if (keyEvent->key() == Qt::Key_Enter)
     {
       m_pendingCloseIsDueToEnter =  true;
       emit hideMe();
       return true;
     }
  }
  return false;
}

void KateViEmulatedCommandBar::editTextChanged(const QString& newText)
{
  qDebug() << "New text: " << newText;
  KTextEditor::Search::SearchOptions searchOptions;
  m_view->setCursorPosition(m_view->doc()->searchText(KTextEditor::Range(m_view->cursorPosition(), m_view->doc()->documentEnd()), newText, searchOptions).first().start());
}
