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

#ifndef __ktexteditor_editor_h__
#define __ktexteditor_editor_h__

#include <kparts/part.h>

namespace KTextEditor
{

/**
 * The Editor class is a wrapper around a document + its unique view.
 * (which means that KTextEditor::Editor can inherit both interfaces for
 * the Document and View (the view interfaces should be applied to the widget()))
 * KTextEditor::Editor is baseclass of KTextEditor::Document, mostly to give the
 * implementors of both interface the chance to use the same class to implement both
 * at once only with minor changes, please don'T cast a KTextEditor::Document to a
 * KTextEditor::Editor and think there must be a widget() !!!!!!!!!!!!
 */

class Editor : public KParts::ReadWritePart
{
  friend class PrivateEditor;

  Q_OBJECT

  public:
    /**
    * Create a new editor widget.
    */
    Editor ( QObject *parent = 0, const char *name = 0  );
    virtual ~Editor ();
    
    unsigned int editorNumber () const;

  private:
    class PrivateEditor *d;
    static unsigned int globalEditorNumber;
    unsigned int myEditorNumber;
};

};

#endif
