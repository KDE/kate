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

#ifndef __ktexteditor_undointerface_h__
#define __ktexteditor_undointerface_h__


namespace KTextEditor
{

/*
*  This is an interface for the KTextEditor::Document class !!!
*/
class UndoInterface
{
  //
  // slots !!!
  //
  public:
    virtual void undo () = 0;

    virtual void redo () = 0;

    virtual void clearUndo () = 0;

    virtual void clearRedo () = 0;

    virtual uint undoCount () const = 0;

    virtual uint redoCount () const = 0;

    virtual uint undoSteps () const = 0;

    virtual void setUndoSteps ( uint steps ) = 0;

  //
  // signals !!!
  //
  public:
    virtual void undoChanged () = 0;
};


};

#endif
