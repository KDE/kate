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

#ifndef __ktexteditor_undointerface_h__
#define __ktexteditor_undointerface_h__

namespace KTextEditor
{

/*
*  This is an interface for the KTextEditor::Document class !!!
*/
class UndoInterface
{
  friend class PrivateUndoInterface;

  public:
    UndoInterface ();
    virtual ~UndoInterface ();

    unsigned int undoInterfaceNumber () const;

  //
  // slots !!!
  //
  public:
    virtual void undo () = 0;

    virtual void redo () = 0;

    virtual void clearUndo () = 0;

    virtual void clearRedo () = 0;

    virtual unsigned int undoCount () const = 0;

    virtual unsigned int redoCount () const = 0;

    virtual unsigned int undoSteps () const = 0;

    virtual void setUndoSteps ( unsigned int steps ) = 0;

  //
  // signals !!!
  //
  public:
    virtual void undoChanged () = 0;

  private:
    class PrivateUndoInterface *d;
    static unsigned int globalUndoInterfaceNumber;
    unsigned int myUndoInterfaceNumber;
};

UndoInterface *undoInterface (class Document *doc);

};

#endif
