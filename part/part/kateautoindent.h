/* This file is part of the KDE libraries
   Copyright (C) 2003 Jesse Yurkovich <yurkjes@iit.edu>

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

#ifndef kate_autoindent_h
#define kate_autoindent_h

#include "katecursor.h"

class KateDocument;

class KateAutoIndent
{
  public:
    KateAutoIndent (KateDocument *doc);
    virtual ~KateAutoIndent ();

    // Update indenter's configuration (indention width etc.)
    void updateConfig ();

    // Called every time a newline character is inserted in the document
    // cur should contain the new cursor position afterwords
    // needContinue is used to determine whether to calculate a continue indent or not
    virtual void processNewline (KateDocCursor &cur, bool needContinue);

    // Called every time a character is inserted into the document
    virtual void processChar (QChar /*c*/) { }

  protected:
    // Determines if the characters open and close are balanced between begin and end
    bool isBalanced (KateDocCursor &begin, const KateDocCursor &end, QChar open, QChar close) const;
    // Skip all whitespace starting at cur and ending at max   spanning lines if newline is set
    // cur is set to the current position afterwards
    bool skipBlanks (KateDocCursor &cur, KateDocCursor &max, bool newline) const;

    // Measures the indention of the current textline marked by cur
    uint measureIndent (KateDocCursor &cur) const;

    // Produces a string with the proper indentation characters for its length
    QString tabString (uint length) const;

    KateDocument *doc;
    uint tabWidth;
    uint indentWidth;
    bool useSpaces;
};

class KateCSmartIndent : public KateAutoIndent
{
  public:
    KateCSmartIndent (KateDocument *doc);
    ~KateCSmartIndent ();

    virtual void processNewline (KateDocCursor &begin, bool needContinue);
    virtual void processChar (QChar c);

  private:
    uint calcIndent (KateDocCursor &begin, bool needContinue);
    uint calcContinue (KateDocCursor &begin, KateDocCursor &end);

    bool allowSemi;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on; smart-indent on;
