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

#include "sessionconfiginterface.h"

#include "document.h"
#include "view.h"
#include "plugin.h"

namespace KTextEditor
{

class PrivateSessionConfigInterface
{
  public:
    PrivateSessionConfigInterface() {}
    ~PrivateSessionConfigInterface() {}
};

};

using namespace KTextEditor;

unsigned int SessionConfigInterface::globalSessionConfigInterfaceNumber = 0;

SessionConfigInterface::SessionConfigInterface()
{
  globalSessionConfigInterfaceNumber++;
  mySessionConfigInterfaceNumber = globalSessionConfigInterfaceNumber++;

  d = new PrivateSessionConfigInterface();
}

SessionConfigInterface::~SessionConfigInterface()
{
  delete d;
}

unsigned int SessionConfigInterface::configInterfaceNumber () const
{
  return mySessionConfigInterfaceNumber;
}

SessionConfigInterface *KTextEditor::sessionConfigInterface (Document *doc)
{                       
  if (!doc)
    return 0;

  return static_cast<SessionConfigInterface*>(doc->qt_cast("KTextEditor::SessionConfigInterface"));
}      

SessionConfigInterface *KTextEditor::sessionConfigInterface (View *view)
{                       
  if (!view)
    return 0;

  return static_cast<SessionConfigInterface*>(view->qt_cast("KTextEditor::SessionConfigInterface"));
}

SessionConfigInterface *KTextEditor::sessionConfigInterface (Plugin *plugin)
{                       
  if (!plugin)
    return 0;

  return static_cast<SessionConfigInterface*>(plugin->qt_cast("KTextEditor::SessionConfigInterface"));
}

SessionConfigInterface *KTextEditor::sessionConfigInterface (ViewPlugin *plugin)
{                       
  if (!plugin)
    return 0;

  return static_cast<SessionConfigInterface*>(plugin->qt_cast("KTextEditor::SessionConfigInterface"));
}
