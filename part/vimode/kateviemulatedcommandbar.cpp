#include "kateviemulatedcommandbar.h"

#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>

KateViEmulatedCommandBar::KateViEmulatedCommandBar(QWidget* parent)
    : KateViewBarWidget(false, parent)
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
}

void KateViEmulatedCommandBar::init()
{
  m_edit->setFocus();
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
       emit hideMe();
       return true;
     }
  }
  return false;
}
