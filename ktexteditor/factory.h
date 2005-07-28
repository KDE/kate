/* This file is part of the KDE libraries
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __ktexteditor_factory_h__
#define __ktexteditor_factory_h__

// our main baseclass of the KTextEditor::Document
#include <kparts/factory.h>

/**
 * Namespace for the KDE Text Editor Interfaces.
 * These interfaces provide easy access to editor parts for the
 * applications embedding them. At the moment they are at least
 * supported by both the Kate Part and the Yzis Part.
 */
namespace KTextEditor
{

class Editor;

/**
 * This class represents the factory of the editor
 * Each KTextEditor part must reimplement this factory,
 * to allow easy access to the editor object
 */
class KTEXTEDITOR_EXPORT Factory : public KParts::Factory
{
  Q_OBJECT

  public:
    /**
     * Factory Constructor
     * @param parent parent object
     */
    Factory ( QObject *parent );

    /**
     * virtual destructor
     */
    virtual ~Factory ();

    /**
     * Retrieve the global editor object, the editor part
     * implementation must ensure that this object lives as long
     * as any factory object exists or any document
     * @return global KTextEditor::Editor object
     */
    virtual Editor *editor () = 0;
};

}

#endif
