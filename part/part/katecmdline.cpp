/* This file is part of the KDE libraries
   Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "katecmdline.h"
#include "katecmdline.moc"

#include "kateview.h"
#include "katefactory.h"

#include "../interfaces/katecmd.h"
#include "../interfaces/document.h"

#include <klocale.h>

// $Id$

KateCmdLine::KateCmdLine (KateView *view)
  : KLineEdit (view)
  , m_view (view)
  , m_msgMode (false)
{
  connect (this, SIGNAL(returnPressed(const QString &)),
           this, SLOT(slotReturnPressed(const QString &)));

  completionObject()->insertItems (KateCmd::instance()->cmds());
}

KateCmdLine::~KateCmdLine ()
{
}

void KateCmdLine::slotReturnPressed ( const QString& cmd )
{
  if (cmd.length () > 0)
  {
    Kate::Command *p = KateCmd::instance()->queryCommand (cmd);

    m_oldText = cmd;
    m_msgMode = true;

    if (p)
    {
      QString msg;

      if (p->exec (m_view, cmd, msg))
      {
        completionObject()->addItem (cmd);
        m_oldText = QString ();

        if (msg.length() > 0)
          setText (i18n ("Success: ") + msg);
        else
          setText (i18n ("Success"));
      }
      else
      {
        if (msg.length() > 0)
          setText (i18n ("Error: ") + msg);
        else
          setText (i18n ("Command \"%1\" failed.").arg (cmd));
      }
    }
    else
      setText (i18n ("No such command: \"%1\"").arg (cmd));
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
