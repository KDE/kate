#include "katecmdline.h"
#include "katecmdline.moc"

#include "kateview.h"
#include "katecmd.h"
#include "katefactory.h"

#include <klocale.h>

// $Id$

KateCmdLine::KateCmdLine (KateView *view)
  : KLineEdit (view)
  , m_view (view)
  , m_msgMode (false)
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

  if (p)
  {
    QString msg;

    if (p->exec (m_view, cmd, msg))
    {
      completionObject()->addItem (cmd);

      if (msg.length() > 0)
        setText (i18n ("Success: ") + msg);
      else
        setText (i18n ("Success"));
    }
    else
    {
      m_oldText = text ();
      m_msgMode = true;

      if (msg.length() > 0)
        setText (i18n ("Error: ") + msg);
      else
        setText (i18n ("Command \"%1\" failed.").arg (m_oldText));
    }
  }
  else
  {
    m_oldText = text ();
    m_msgMode = true;
    setText (i18n ("No such command: \"%1\"").arg (m_oldText));
  }

  m_view->setFocus ();
}

void KateCmdLine::focusInEvent ( QFocusEvent *ev )
{
  if (m_msgMode)
  {
    m_msgMode = false;
    setText (m_oldText);
  }

  KLineEdit::focusInEvent (ev);
}
