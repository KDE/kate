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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __ktexteditor_editor_h__
#define __ktexteditor_editor_h__

#include <kparts/part.h>

/**
 * KTextEditor is KDE's standard text editing KPart interface.
 */
namespace KTextEditor
{

/**
 * This is a simplfied version of the Document & View classes
 * Usage: Load it, merge it's gui + be happy
 * Extensibility: Use the Document / View classes if you want
 * advanced features, interfaces, etc. This class is just a good text editor
 * widget replacement for applications which just need an embedded text edtor
 * and are not interested in using advanced interfaces.
 */

class KTEXTEDITOR_EXPORT Editor : public KParts::ReadWritePart
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

KTEXTEDITOR_EXPORT Editor *createEditor ( const char* libname, QWidget *parentWidget = 0, const char *widgetName = 0, QObject *parent = 0, const char *name = 0 );

}

#endif
