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
    KateTextCursor() : m_line(0), m_col(0) {};
    KateTextCursor(int line, int col) : m_line(line), m_col(col) {};

    friend bool operator==(const KateTextCursor& c1, const KateTextCursor& c2)
      { return c1.m_line == c2.m_line && c1.m_col == c2.m_col; }

    friend bool operator!=(const KateTextCursor& c1, const KateTextCursor& c2)
      { return !(c1 == c2); }

    friend bool operator>(const KateTextCursor& c1, const KateTextCursor& c2)
      { return c1.m_line > c2.m_line || (c1.m_line == c2.m_line && c1.m_col > c2.m_col); }

    friend bool operator>=(const KateTextCursor& c1, const KateTextCursor& c2)
      { return c1.m_line > c2.m_line || (c1.m_line == c2.m_line && c1.m_col >= c2.m_col); }

    friend bool operator<(const KateTextCursor& c1, const KateTextCursor& c2)
      { return !(c1 >= c2); }

    friend bool operator<=(const KateTextCursor& c1, const KateTextCursor& c2)
      { return !(c1 > c2); }

    inline void pos(int *pline, int *pcol) const {
      if(pline) *pline = m_line;
      if(pcol) *pcol = m_col;
    }

    inline bool subjectToChangeAt(const KateTextCursor & c) const {
      return m_line == c.line() && m_col > c.col();
    }

    inline int line() const { return m_line; };
    inline int col() const { return m_col; };

    virtual void setLine(int line) { m_line = line; };
    virtual void setCol(int col) { m_col = col; };
    virtual void setPos(const KateTextCursor& pos) { m_line = pos.line(); m_col = pos.col(); };
    virtual void setPos(int line, int col) { m_line = line; m_col = col; };

  protected:
    int m_line;
    int m_col;
};

/**
 * We need something a bit special for the internal view implementation:
 * a cursor with a mode to allow setting to new settings, while still returning the old
 * settings, unless instructed to return the new ones.
 *
 * Works around a design issue with emitting textChanged() and apps which access the
 * cursor immediately after and expect it to be updated already.
 */
class KateMutableTextCursor : public KateTextCursor
{
public:
    KateMutableTextCursor();

    void setImmutable(bool immutable);
    const KateTextCursor& mutableCursor() const;

    virtual void setLine(int line);
    virtual void setCol(int col);
    virtual void setPos(const KateTextCursor& pos);
    virtual void setPos(int line, int col);

private:
    bool m_immutable;
    KateTextCursor m_newSettings;
};

// This doesn't belong here
class BracketMark
{
  public:
    BracketMark() : valid( false ) {}
    bool valid;
    uint startLine;
    uint startCol;
    uint startX;
    uint startW;
    uint endLine;
    uint endCol;
    uint endX;
    uint endW;
};

/**
  Cursor class with a pointer to its document.
*/
class KateDocCursor : public KateTextCursor
{
  public:

    KateDocCursor(KateDocument *doc);
    KateDocCursor(int line, int col, KateDocument *doc);
    virtual ~KateDocCursor();

    bool validPosition(uint line, uint col);
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

    bool nextNonSpaceChar();
    bool previousNonSpaceChar();

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
