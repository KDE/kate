/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann (cullmann@kde.org)

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
*/

#include "document.h"
#include "view.h"
#include "editor.h"

#include "document.moc"
#include "view.moc"
#include "editor.moc"

#include "editinterface.h"
#include "selectioninterface.h"
#include "blockselectioninterface.h"
#include "cursorinterface.h"
#include "undointerface.h"
#include "clipboardinterface.h"
#include "popupmenuinterface.h"
#include "viewcursorinterface.h"
#include "searchinterface.h"
#include "highlightinginterface.h"
#include "codecompletioninterface.h"
#include "configinterface.h"
#include "markinterface.h"
#include "printinterface.h"

using namespace KTextEditor;

namespace KTextEditor
{
  class PrivateDocument
  {
  public:
    PrivateDocument ()
    {
    }

    ~PrivateDocument()
    {
    }
  };
  
  class PrivateView
  {
  public:
    PrivateView ()
    {
    }

    ~PrivateView()
    {
    }
  };

  class PrivateEditor
  {
  public:
    PrivateEditor ()
    {
    }

    ~PrivateEditor()
    {
    }
  };
};

unsigned int Document::globalDocumentNumber = 0;
unsigned int View::globalViewNumber = 0;
unsigned int Editor::globalEditorNumber = 0;

Document::Document( QObject *parent, const char *name ) : KTextEditor::Editor (parent, name )
{
  globalDocumentNumber++;
  myDocumentNumber = globalDocumentNumber;
}

Document::~Document()
{
}

unsigned int Document::documentNumber () const
{
  return myDocumentNumber;
}

View::View( Document *, QWidget *parent, const char *name ) : QWidget( parent, name )
{
  globalViewNumber++;
  myViewNumber = globalViewNumber;
}

View::~View()
{
}

unsigned int View::viewNumber () const
{
  return myViewNumber;
}

Editor::Editor( QObject *parent, const char *name ) : KParts::ReadWritePart( parent, name )
{
  globalEditorNumber++;
  myEditorNumber = globalEditorNumber;
}

Editor::~Editor()
{
}

unsigned int Editor::editorNumber () const
{
  return myEditorNumber;
}