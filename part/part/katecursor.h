/* This file is part of the KDE libraries
   Copyright (C) 2002 Christian Couder <christian@kdevelop.org>
   Copyright (C) 2001, 2003 Christoph Cullmann <cullmann@kde.org>
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
    virtual ~KateTextCursor () {};

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
      
#ifndef Q_WS_WIN //not needed
    friend void qSwap(KateTextCursor & c1, KateTextCursor & c2) {
      KateTextCursor tmp = c1;
      c1 = c2;
      c2 = tmp;
    }
#endif

    inline void pos(int *pline, int *pcol) const {
      if(pline) *pline = m_line;
      if(pcol) *pcol = m_col;
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
  Cursor class with a pointer to its document.
*/
class KateDocCursor : public KateTextCursor
{
  public:
    KateDocCursor(KateDocument *doc);
    KateDocCursor(int line, int col, KateDocument *doc);
    virtual ~KateDocCursor() {};

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

    uchar currentAttrib() const;

    /**
      Find the position (line and col) of the next char
      that is not a space. If found KateDocCursor points to the
      found character. Otherwise to a invalid Position such that
      validPosition() returns false.
      @return True if the specified or a following character is not a space
               Otherwise false.
    */
    bool nextNonSpaceChar();

    /**
      Find the position (line and col) of the previous char
      that is not a space. If found KateDocCursor points to the
      found character. Otherwise to a invalid Position such that
      validPosition() returns false.
      @return True if the specified or a preceding character is not a space
               Otherwise false.
    */
    bool previousNonSpaceChar();

  protected:
    KateDocument *m_doc;
};

class KateRange
{
  public:
    KateRange () {};
    virtual ~KateRange () {};

    virtual bool isValid() const = 0;
    virtual KateTextCursor& start() = 0;
    virtual KateTextCursor& end() = 0;
    virtual const KateTextCursor& start() const = 0;
    virtual const KateTextCursor& end() const = 0;
};

class KateTextRange : public KateRange
{
  public:
    KateTextRange()
      : m_valid(false)
    {
    };

    KateTextRange(int startline, int startcol, int endline, int endcol)
      : m_start(startline, startcol)
      , m_end(endline, endcol)
      , m_valid(true)
    {
      normalize();
    };

    KateTextRange(const KateTextCursor& start, const KateTextCursor& end)
      : m_start(start)
      , m_end(end)
      , m_valid(true)
    {
      normalize();
    };

    virtual ~KateTextRange () {};

    virtual bool isValid() const { return m_valid; };
    void setValid(bool valid) { 
      m_valid = valid; 
      if( valid )
        normalize(); 
    };

    virtual KateTextCursor& start() { return m_start; };
    virtual KateTextCursor& end() { return m_end; };
    virtual const KateTextCursor& start() const { return m_start; };
    virtual const KateTextCursor& end() const { return m_end; };
    
    /* if range is not valid, the result is undefined
      if cursor is before start -1 is returned, if cursor is within range 0 is returned if cursor is after end 1 is returned*/
    inline int cursorInRange(const KateTextCursor &cursor) const {
      return ((cursor<m_start)?(-1):((cursor>m_end)?1:0));
    }
    
    inline void normalize() {
      if( m_start > m_end )
        qSwap(m_start, m_end);
    }
    
  protected:
    KateTextCursor m_start, m_end;
    bool m_valid;
};


class KateBracketRange : public KateTextRange
{
  public:
    KateBracketRange()
      : KateTextRange()
      , m_minIndent(0)
    {
    };
    
    KateBracketRange(int startline, int startcol, int endline, int endcol, int minIndent)
      : KateTextRange(startline, startcol, endline, endcol)
      , m_minIndent(minIndent)
    {
    };
    
    KateBracketRange(const KateTextCursor& start, const KateTextCursor& end, int minIndent)
      : KateTextRange(start, end)
      , m_minIndent(minIndent)
    {
    };
    
    int getMinIndent() const
    {
      return m_minIndent;
    }
    
    void setIndentMin(int m)
    {
      m_minIndent = m;
    }
    
  protected:
    int m_minIndent;
};


#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
