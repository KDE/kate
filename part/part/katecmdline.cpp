#include "katecmdline.h"
#include "katecmdline.moc"

#include "kateview.h"
#include "katecmd.h"
#include "katefactory.h"

// $Id$

KateCmdLine::KateCmdLine (KateView *view)
  : KLineEdit (view)
  , m_view (view)
{
  connect (this, SIGNAL(returnPressed(const QString &)),
           this, SLOT(slotReturnPressed(const QString &)));

  completionObject()->insertItems (KateFactory::cmd()->cmds());
}

KateCmdLine::~KateCmdLine ()
{
}

void KateCmdLine::slotReturnPressed ( const QString& cmd )
{
  KateCmdParser *p = KateFactory::cmd()->query (cmd);
  QString error;

  if (p)
  {
    if (p->exec (m_view, cmd, error))
    {
      completionObject()->addItem (cmd);
      clear ();
    }
  }

  m_view->setFocus ();
}
