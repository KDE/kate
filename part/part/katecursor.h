/* This file is part of the KDE libraries
   Copyright (C) 2002 Christian Couder <christian@kdevelop.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>   
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef kate_cursor_h
#define kate_cursor_h

#include <config.h>
#include "../interfaces/document.h"

class KateTextCursor
{
  public:
    KateTextCursor() : line( 0 ), col( 0 ) {};
    KateTextCursor( int _line, int _col ) : line( _line ), col( _col ) {};

    friend bool operator==( const KateTextCursor& c1, const KateTextCursor& c2 )
      { return c1.line == c2.line && c1.col == c2.col; }

    inline void pos ( uint *pline, uint *pcol ) const {
      if(pline) *pline = line;
      if(pcol) *pcol = col;
    }

    inline void setPos ( uint _line, uint _col ) {
      line = _line;
      col = _col;
    }    

  public:
    int line;
    int col;
};

class BracketMark
{
  public:
    KateTextCursor cursor;
    int sXPos;
    int eXPos;
};

class KateCursor : public KateTextCursor, public Kate::Cursor
{
  public:
    KateCursor (class KateDocument *doc);
    ~KateCursor ();

    void position ( uint *line, uint *col ) const;

    bool setPosition ( uint line, uint col );

    bool insertText ( const QString& text );

    bool removeText ( uint numberOfCharacters );

    QChar currentChar () const;

  private:
    class KateDocument *myDoc;
};

#endif
