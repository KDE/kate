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
 * this is the "easy to use" wrapper around the ktexteditor interfaces
 * this interfaces provides a plain kpart with a single view, you can use the
 * ktexteditor::document interfaces for it and the ktexteditor::view interfaces
 * for its view() widget, this class is just for "easy of use", we strongly recommend
 * to support the real ktexteditor::document/view interfaces, too, as stuff like plugins
 * will need them.
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
