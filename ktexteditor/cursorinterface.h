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

#ifndef __ktexteditor_cursorinterface_h__
#define __ktexteditor_cursorinterface_h__

namespace KTextEditor
{

class Cursor
{
  public:
    virtual void position ( uint *line, uint *col ) const = 0;

    virtual bool setPosition ( uint line, uint col ) = 0;

    virtual bool insertText ( const QString& text ) = 0;

    virtual bool removeText ( uint numberOfCharacters ) = 0;

    virtual QChar currentChar () const = 0;
};

/*
*  This is an interface for the KTextEditor::Document class !!!
*/
class CursorInterface
{
  friend class PrivateCursorInterface;
  public:
    /**
    * Create a new cursor object
    */
    virtual Cursor *createCursor ( ) = 0;

    /*
    * Accessor to the list of views.
    */
    virtual QPtrList<Cursor> cursors () const = 0;
    private:
    	class PrivateCursorInterface *d;
};

};

#endif
