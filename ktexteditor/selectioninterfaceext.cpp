/* This file is part of the KDE project
   Copyright (C) 2002 Anders Lund <anders@alweb.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
   
   $Id:$
*/

#include "selectioninterfaceext.h"
#include "document.h"
#include <dcopclient.h>

using namespace KTextEditor;

//BEGIN KTextEditor::SelectionInterfaceExt
class KTextEditor::PrivateSelectionInterfaceExt {
  public:
    PrivateSelectionInterfaceExt() : interface( 0 ) {};
    ~PrivateSelectionInterfaceExt() {};
    
    SelectionExtDCOPInterface *interface;
};

unsigned int SelectionInterfaceExt::globalSelectionInterfaceExtNumber = 0;

SelectionInterfaceExt::SelectionInterfaceExt()
  : d ( new PrivateSelectionInterfaceExt )
{
  globalSelectionInterfaceExtNumber++;
  mySelectionInterfaceExtNumber = globalSelectionInterfaceExtNumber;
  QString name = "SelectionInterfaceExt#" + QString::number(mySelectionInterfaceExtNumber);
  d->interface = new SelectionExtDCOPInterface(this, name.latin1());
}

SelectionInterfaceExt::~SelectionInterfaceExt()
{
  delete d->interface;
  delete d;
}

unsigned int SelectionInterfaceExt::selectionInterfaceExtNumber () const
{
  return mySelectionInterfaceExtNumber;
}

SelectionInterfaceExt *KTextEditor::selectionInterfaceExt (Document *doc)
{
  if (!doc)
    return 0;

  return static_cast<SelectionInterfaceExt*>(doc->qt_cast("KTextEditor::SelectionInterfaceExt"));
}
//END KTextEditor::SelectionInterfaceExt

//BEGIN KTextEditor::SelectionExtDCOPInterface
SelectionExtDCOPInterface::SelectionExtDCOPInterface(
                SelectionInterfaceExt *parent, const char* name )
  : DCOPObject( name ),
    m_parent( parent )
{
}

SelectionExtDCOPInterface::~SelectionExtDCOPInterface()
{
}

int SelectionExtDCOPInterface::selStartLine()
{
  return m_parent->selStartLine();
}

int SelectionExtDCOPInterface::selStartCol()
{
  return m_parent->selStartCol();
}

int SelectionExtDCOPInterface::selEndLine()
{
  return m_parent->selEndLine();
}

int SelectionExtDCOPInterface::selEndCol()
{
  return m_parent->selEndCol();
}
//END KTextEditor::SelectionExtDCOPInterface
