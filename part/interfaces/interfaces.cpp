/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

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

// $Id$

#include "document.h"
#include "document.moc"

#include "view.h"
#include "view.moc"

#include "katecmd.h"

namespace Kate
{

bool Document::s_openErrorDialogsActivated = true;

Document::Document () : KTextEditor::Document (0L, "Kate::Document")
{
}

Document::~Document ()
{
}

void Document::setOpenErrorDialogsActivated (bool on)
{
  s_openErrorDialogsActivated = on;
}

bool Document::registerCommand (Command *cmd)
{
  return KateCmd::instance()->registerCommand (cmd);
}

bool Document::unregisterCommand (Command *cmd)
{
  return KateCmd::instance()->registerCommand (cmd);
}

Command *Document::queryCommand (const QString &cmd)
{
  return KateCmd::instance()->queryCommand (cmd);
}

View::View ( KTextEditor::Document *doc, QWidget *parent, const char *name ) : KTextEditor::View (doc, parent, name)
{
}

View::~View ()
{
}

void ConfigPage::slotChanged()
{
  emit changed();
}


Document *document (KTextEditor::Document *doc)
{
  if (!doc)
    return 0;

  return static_cast<Document*>(doc->qt_cast("Kate::Document"));
}

Document *createDocument ( QObject *parent, const char *name )
{
  return (Document* ) KTextEditor::createDocument ("libkatepart", parent, name);
}

View *view (KTextEditor::View *view)
{
  if (!view)
    return 0;

  return static_cast<View*>(view->qt_cast("Kate::View"));
}

}
