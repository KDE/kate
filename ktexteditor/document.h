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

#ifndef __ktexteditor_document_h__
#define __ktexteditor_document_h__

#include "editor.h"

namespace KTextEditor
{

class Document : public KTextEditor::Editor
{
  friend class PrivateDocument;

  Q_OBJECT

  public:
    Document ( QObject *parent = 0, const char *name = 0 );
    virtual ~Document ();

    unsigned int documentNumber () const;

    /**
    * Create a view that will display the document data. You can create as many
    * views as you like. When the user modifies data in one view then all other
    * views will be updated as well.
    */
    virtual class View *createView ( QWidget *parent, const char *name = 0 ) = 0;

    /*
    * Accessor to the list of views.
    */
    virtual QPtrList<class View> views () const = 0;

  private:
    class PrivateDocument *d;
    static unsigned int globalDocumentNumber;
    unsigned int myDocumentNumber;
};
                    
Document *createDocument ( const char* libname, QObject *parent = 0, const char *name = 0 );

};

#endif
