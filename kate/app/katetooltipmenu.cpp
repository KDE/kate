#include "katetooltipmenu.h"
#include "katetooltipmenu.moc"

#include <QEvent>
#include <QLabel>
#include <QToolTip>
#include <QApplication>
#include <QDesktopWidget>
#include <kdebug.h>

KateToolTipMenu::KateToolTipMenu(QWidget *parent): QMenu(parent), m_currentAction(0), m_toolTip(0)
{
  connect(this, SIGNAL(hovered(QAction*)), this, SLOT(slotHovered(QAction*)));
}

KateToolTipMenu::~KateToolTipMenu()
{}

bool KateToolTipMenu::event(QEvent* event)
{
#if 0
  if (event->type() == QEvent::ToolTip)
  {
    if (m_currentAction)
    {

      //tmp->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred));

      event->setAccepted(true);
      return true;
    }
  }
#endif
  return QMenu::event(event);
}

void KateToolTipMenu::slotHovered(QAction* a)
{
  m_currentAction = a;
  if (!a)
  {
    delete m_toolTip;
    m_toolTip = 0;
    return;
  }
  if (!m_toolTip) m_toolTip = new QLabel(this, Qt::ToolTip);
  m_toolTip->setText(m_currentAction->toolTip());
  QRect fg = frameGeometry();
  m_toolTip->ensurePolished();
  m_toolTip->resize(m_toolTip->sizeHint());
  m_toolTip->setPalette(QToolTip::palette());
  kDebug() << "fg.left" << fg.left() << endl;
  kDebug() << "m_tooltip->width()" << m_toolTip->width() << endl;
  kDebug() << "xpos" << fg.left() - m_toolTip->width() << endl;
  QRect fgl = m_toolTip->frameGeometry();
  QRect dr = QApplication::desktop()->availableGeometry(this);
  int posx;
  if ( (fg.right() + fgl.width() + 1) > dr.right())
    posx = fg.left() - m_toolTip->width();
  else
    posx = fg.right() + 1;
  int posy;
  if ((fg.top() + fgl.height()) > dr.bottom())
    posy = fg.bottom() - fgl.height();
  else
    posy = fg.top();
  //m_toolTip->move(fg.left()-m_toolTip->width(),fg.top());
  m_toolTip->move(posx, posy);
  m_toolTip->show();

}
// kate: space-indent on; indent-width 2; replace-tabs on;

