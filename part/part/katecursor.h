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

class KateDocument;


/**
  Simple cursor class with no document pointer.
*/
class KateTextCursor
{
  public:
    KateTextCursor() : line(0), col(0) {};
    KateTextCursor(int _line, int _col) : line(_line), col(_col) {};

    friend bool operator==(const KateTextCursor& c1, const KateTextCursor& c2)
      { return c1.line == c2.line && c1.col == c2.col; }

    friend bool operator!=(const KateTextCursor& c1, const KateTextCursor& c2)
      { return !(c1 == c2); }

    friend bool operator>(const KateTextCursor& c1, const KateTextCursor& c2)
      { return c1.line > c2.line || (c1.line == c2.line && c1.col > c2.col); }

    friend bool operator>=(const KateTextCursor& c1, const KateTextCursor& c2)
      { return c1.line > c2.line || (c1.line == c2.line && c1.col >= c2.col); }

    friend bool operator<(const KateTextCursor& c1, const KateTextCursor& c2)
      { return !(c1 >= c2); }

    friend bool operator<=(const KateTextCursor& c1, const KateTextCursor& c2)
      { return !(c1 > c2); }

    inline void pos(int *pline, int *pcol) const {
      if(pline) *pline = line;
      if(pcol) *pcol = col;
    }

    inline void setPos(int _line, int _col) {
      line = _line;
      col = _col;
    }

    inline void setPos(const KateTextCursor & c) {
      line = c.line;
      col = c.col;
    }

    bool subjectToChangeAt(const KateTextCursor & c) const {
      return line == c.line && col > c.col;
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

/**
  Cursor class with a pointer to its document.
*/
class KateDocCursor : public KateTextCursor
{
  public:

    KateDocCursor(KateDocument *doc);
    KateDocCursor(int _line, int _col, KateDocument *doc);
    ~KateDocCursor();

    bool validPosition(uint _line, uint _col);
    bool validPosition();

    bool gotoNextLine();
    bool gotoPreviousLine();
    bool gotoEndOfNextLine();
    bool gotoEndOfPreviousLine();

    int nbCharsOnLineAfter();
    bool moveForward(uint nbChar);
    bool moveBackward(uint nbChar);

    // KTextEditor::Cursor interface
    void position(uint *line, uint *col) const;
    bool setPosition(uint line, uint col);
    bool insertText(const QString& text);
    bool removeText(uint numberOfCharacters);
    QChar currentChar() const;

  protected:
    KateDocument *m_doc;
};

/**
  Complex cursor class implementing the KTextEditor::Cursor interface
  through Kate::Cursor using KateDocCursor features.

  (KTextEditor::Cursor objects should hold their position, which
  means: You set some cursor to line 10/col 10, the user deletes lines
  2-4 -> the cursor should move automagically to line 7 to stay at
  it's old position in the text. At least it was thought so.)
*/
class KateCursor : public KateDocCursor, public Kate::Cursor
{
  public:
    KateCursor(KateDocument *doc);
    ~KateCursor();

    // KTextEditor::Cursor interface
    void position(uint *line, uint *col) const;
    bool setPosition(uint line, uint col);
    bool insertText(const QString& text);
    bool removeText(uint numberOfCharacters);
    QChar currentChar() const;
};

#endif
