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

}

void KateViEmulatedCommandBar::init()
{
  m_edit->setFocus();
}
