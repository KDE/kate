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

#ifndef __ktexteditor_editor_h__
#define __ktexteditor_editor_h__

#include <kparts/part.h>

namespace KTextEditor
{

/**
 * KTextEditor::Editor
 * This is just the dumb version of a KTextEditor::Document + View
 * Usage: Load it, merge it's gui + be happy
 * Extensibility: Use the KTextEditor::Document/View classes if you want
 * advanced features, interfaces, ..., this class ist just a good edit widget
 * replacement for apps which just need any kind of embedded edtor and
 * don't care about using doc/view + interfaces
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
